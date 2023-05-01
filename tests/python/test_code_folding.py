"""Tests for reading/writing Protocol Buffers"""
import itertools
import os

import pytest

import tl2cgen

from .metadata import format_libpath_for_example_model, load_example_model
from .util import check_predictor, os_compatible_toolchains


@pytest.mark.parametrize("code_folding_factor", [0.0, 1.0, 2.0, 3.0])
@pytest.mark.parametrize(
    "dataset,toolchain",
    list(
        itertools.product(
            ["dermatology", "toy_categorical"], os_compatible_toolchains()
        )
    ),
)
def test_code_folding(tmpdir, annotation, dataset, toolchain, code_folding_factor):
    """Test suite for testing code folding feature"""
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)
    annotation_path = os.path.join(tmpdir, "annotation.json")

    if annotation[dataset] is None:
        annotation_path = None
    else:
        with open(annotation_path, "w", encoding="UTF-8") as f:
            f.write(annotation[dataset])

    params = {
        "annotate_in": (annotation_path if annotation_path else "NULL"),
        "quantize": 1,
        "parallel_comp": model.num_tree,
        "code_folding_req": code_folding_factor,
    }

    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params=params, verbose=True
    )
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    check_predictor(predictor, dataset)
