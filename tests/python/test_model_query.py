"""Tests for rich model query functions"""

import collections

import numpy as np
import pytest

import tl2cgen

from .metadata import format_libpath_for_example_model, load_example_model
from .util import os_compatible_toolchains, os_platform

ModelFact = collections.namedtuple(
    "ModelFact",
    "num_tree num_feature num_target num_class threshold_type leaf_output_type",
)
MODEL_FACTS = {
    "mushroom": ModelFact(2, 127, 1, [1], "float32", "float32"),
    "dermatology": ModelFact(60, 33, 1, [6], "float32", "float32"),
    "toy_categorical": ModelFact(30, 2, 1, [1], "float64", "float64"),
    "sparse_categorical": ModelFact(1, 5057, 1, [1], "float64", "float64"),
}


@pytest.mark.parametrize(
    "dataset", ["mushroom", "dermatology", "toy_categorical", "sparse_categorical"]
)
def test_model_query(tmpdir, dataset):
    """Test all query functions for every example model"""
    if dataset == "sparse_categorical":
        if os_platform() == "windows":
            pytest.xfail("MSVC cannot handle long if conditional")
        elif os_platform() == "osx":
            pytest.xfail("Apple Clang cannot handle long if conditional")

    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)
    assert model.num_feature == MODEL_FACTS[dataset].num_feature
    assert model.num_tree == MODEL_FACTS[dataset].num_tree

    # pylint: disable=R0801
    toolchain = os_compatible_toolchains()[0]
    tl2cgen.export_lib(
        model,
        toolchain=toolchain,
        libpath=libpath,
        params={"quantize": 1, "parallel_comp": model.num_tree},
        verbose=True,
    )
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == MODEL_FACTS[dataset].num_feature
    assert predictor.num_target == MODEL_FACTS[dataset].num_target
    np.testing.assert_array_equal(predictor.num_class, MODEL_FACTS[dataset].num_class)
    assert predictor.threshold_type == MODEL_FACTS[dataset].threshold_type
    assert predictor.leaf_output_type == MODEL_FACTS[dataset].leaf_output_type
