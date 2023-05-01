"""Suite of basic tests"""
import itertools
import os
import pathlib
import subprocess
import sys
from zipfile import ZipFile

import pytest
from scipy.sparse import csr_matrix

import tl2cgen

from .metadata import (
    example_model_db,
    format_libpath_for_example_model,
    load_example_model,
)
from .util import check_predictor, does_not_raise, os_compatible_toolchains, os_platform


@pytest.mark.parametrize(
    "dataset,use_annotation,parallel_comp,quantize,toolchain",
    list(
        itertools.product(
            ["mushroom", "dermatology"],
            [True, False],
            [None, 4],
            [True, False],
            os_compatible_toolchains(),
        )
    )
    + [
        ("toy_categorical", False, 30, True, os_compatible_toolchains()[0]),
    ],
)
def test_basic(
    tmpdir, annotation, dataset, use_annotation, quantize, parallel_comp, toolchain
):
    # pylint: disable=too-many-arguments
    """Basic test for C codegen"""
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)
    annotation_path = os.path.join(tmpdir, "annotation.json")

    if use_annotation:
        if annotation[dataset] is None:
            pytest.skip("No training data available. Skipping annotation")
        with open(annotation_path, "w", encoding="UTF-8") as f:
            f.write(annotation[dataset])

    params = {
        "annotate_in": (annotation_path if use_annotation else "NULL"),
        "quantize": (1 if quantize else 0),
        "parallel_comp": (parallel_comp if parallel_comp else 0),
    }
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params=params, verbose=True
    )
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    check_predictor(predictor, dataset)


@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
@pytest.mark.parametrize("use_elf", [True, False])
@pytest.mark.parametrize("dataset", ["mushroom", "dermatology", "toy_categorical"])
def test_failsafe_compiler(tmpdir, dataset, use_elf, toolchain):
    """Test 'failsafe' compiler"""
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)

    params = {"dump_array_as_elf": (1 if use_elf else 0)}

    is_linux = sys.platform.startswith("linux")
    # Expect TL2cgen to throw error if we try to use dump_array_as_elf on non-Linux OS
    # Also, failsafe compiler is only available for XGBoost models
    if ((not is_linux) and use_elf) or example_model_db[dataset].format != "xgboost":
        expect_raises = pytest.raises(tl2cgen.TL2cgenError)
    else:
        expect_raises = does_not_raise()
    with expect_raises:
        tl2cgen.export_lib(
            model,
            compiler="failsafe",
            toolchain=toolchain,
            libpath=libpath,
            params=params,
            verbose=True,
        )
        predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
        check_predictor(predictor, dataset)


@pytest.mark.skipif(os_platform() == "windows", reason="Make unavailable on Windows")
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
@pytest.mark.parametrize("dataset", ["mushroom", "dermatology", "toy_categorical"])
def test_srcpkg(tmpdir, dataset, toolchain):
    """Test feature to export a source tarball"""
    pkgpath = pathlib.Path(tmpdir) / "srcpkg.zip"
    model = load_example_model(dataset)
    tl2cgen.export_srcpkg(
        model,
        toolchain=toolchain,
        pkgpath=pkgpath,
        libname=example_model_db[dataset].libname,
        params={"parallel_comp": 4},
        verbose=True,
    )
    with ZipFile(pkgpath, "r") as zip_ref:
        zip_ref.extractall(tmpdir)
    nproc = os.cpu_count()
    subprocess.check_call(
        ["make", "-C", example_model_db[dataset].libname, f"-j{nproc}"], cwd=tmpdir
    )

    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    check_predictor(predictor, dataset)


@pytest.mark.parametrize("dataset", ["mushroom", "dermatology", "toy_categorical"])
def test_srcpkg_cmake(tmpdir, dataset):  # pylint: disable=R0914
    """Test feature to export a source tarball"""
    pkgpath = pathlib.Path(tmpdir) / "srcpkg.zip"
    model = load_example_model(dataset)
    tl2cgen.export_srcpkg(
        model,
        toolchain="cmake",
        pkgpath=pkgpath,
        libname=example_model_db[dataset].libname,
        params={"parallel_comp": 4},
        verbose=True,
    )
    with ZipFile(pkgpath, "r") as zip_ref:
        zip_ref.extractall(tmpdir)
    build_dir = pathlib.Path(tmpdir) / example_model_db[dataset].libname / "build"
    build_dir.mkdir()
    nproc = os.cpu_count()
    win_opts = ["-A", "x64"] if os_platform() == "windows" else []
    subprocess.check_call(["cmake", ".."] + win_opts, cwd=build_dir)
    subprocess.check_call(
        ["cmake", "--build", ".", "--config", "Release", "--parallel", str(nproc)],
        cwd=build_dir,
    )

    predictor = tl2cgen.Predictor(libpath=build_dir, verbose=True)
    check_predictor(predictor, dataset)


def test_deficient_matrix(tmpdir):
    """Test if TL2cgen correctly handles sparse matrix with fewer columns than the training data
    used for the model. In this case, the matrix should be padded with zeros."""
    libpath = format_libpath_for_example_model("mushroom", prefix=tmpdir)
    model = load_example_model("mushroom")
    toolchain = os_compatible_toolchains()[0]
    tl2cgen.export_lib(
        model,
        toolchain=toolchain,
        libpath=libpath,
        params={"quantize": 1},
        verbose=True,
    )

    X = csr_matrix(([], ([], [])), shape=(3, 3))
    dmat = tl2cgen.DMatrix(X, dtype="float32")
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == 127
    predictor.predict(dmat)  # should not crash


def test_too_wide_matrix(tmpdir):
    """Test if TL2cgen correctly handles sparse matrix with more columns than the training data
    used for the model. In this case, an exception should be thrown"""
    libpath = format_libpath_for_example_model("mushroom", prefix=tmpdir)
    model = load_example_model("mushroom")
    toolchain = os_compatible_toolchains()[0]
    tl2cgen.export_lib(
        model,
        toolchain=toolchain,
        libpath=libpath,
        params={"quantize": 1},
        verbose=True,
    )

    X = csr_matrix(([], ([], [])), shape=(3, 1000))
    dmat = tl2cgen.DMatrix(X, dtype="float32")
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == 127
    pytest.raises(tl2cgen.TL2cgenError, predictor.predict, dmat)
