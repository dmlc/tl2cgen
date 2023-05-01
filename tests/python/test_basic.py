"""Suite of basic tests"""
import itertools
import os

import pytest
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

from .metadata import dataset_db
from .util import check_predictor, os_compatible_toolchains, os_platform


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

    if dataset == "letor" and os_platform() == "windows":
        pytest.xfail("export_lib() is too slow for letor on MSVC")

    libpath = os.path.join(tmpdir, dataset_db[dataset].libname + _libext())
    model = treelite.Model.load(
        dataset_db[dataset].model, model_format=dataset_db[dataset].format
    )
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
