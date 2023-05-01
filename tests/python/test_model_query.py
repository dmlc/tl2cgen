"""Tests for rich model query functions"""

import collections

import pytest

import tl2cgen

from .metadata import format_libpath_for_example_model, load_example_model
from .util import os_compatible_toolchains, os_platform

ModelFact = collections.namedtuple(
    "ModelFact",
    "num_tree num_feature num_class pred_transform global_bias sigmoid_alpha ratio_c "
    "threshold_type leaf_output_type",
)
MODEL_FACTS = {
    "mushroom": ModelFact(2, 127, 1, "sigmoid", 0.0, 1.0, 1.0, "float32", "float32"),
    "dermatology": ModelFact(60, 33, 6, "softmax", 0.5, 1.0, 1.0, "float32", "float32"),
    "toy_categorical": ModelFact(
        30, 2, 1, "identity", 0.0, 1.0, 1.0, "float64", "float64"
    ),
    "sparse_categorical": ModelFact(
        1, 5057, 1, "sigmoid", 0.0, 1.0, 1.0, "float64", "float64"
    ),
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
    assert model.num_class == MODEL_FACTS[dataset].num_class
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
    assert predictor.num_class == MODEL_FACTS[dataset].num_class
    assert predictor.pred_transform == MODEL_FACTS[dataset].pred_transform
    assert predictor.global_bias == MODEL_FACTS[dataset].global_bias
    assert predictor.sigmoid_alpha == MODEL_FACTS[dataset].sigmoid_alpha
    assert predictor.ratio_c == MODEL_FACTS[dataset].ratio_c
    assert predictor.threshold_type == MODEL_FACTS[dataset].threshold_type
    assert predictor.leaf_output_type == MODEL_FACTS[dataset].leaf_output_type
