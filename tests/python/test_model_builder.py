"""Tests for model builder interface"""
import os
import pathlib

import numpy as np
import pytest
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

from .util import os_compatible_toolchains


@pytest.mark.parametrize("test_round_trip", [True, False])
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_node_insert_delete(tmpdir, toolchain, test_round_trip):
    # pylint: disable=R0914
    """Test ability to add and remove nodes"""
    num_feature = 3
    builder = treelite.ModelBuilder(num_feature=num_feature)
    builder.append(treelite.ModelBuilder.Tree())
    builder[0][1].set_root()
    builder[0][1].set_numerical_test_node(
        feature_id=2,
        opname="<",
        threshold=-0.5,
        default_left=True,
        left_child_key=5,
        right_child_key=10,
    )
    builder[0][5].set_leaf_node(-1)
    builder[0][10].set_numerical_test_node(
        feature_id=0,
        opname="<=",
        threshold=0.5,
        default_left=False,
        left_child_key=7,
        right_child_key=8,
    )
    builder[0][7].set_leaf_node(0.0)
    builder[0][8].set_leaf_node(1.0)
    del builder[0][1]
    del builder[0][5]
    builder[0][5].set_categorical_test_node(
        feature_id=1,
        left_categories=[1, 2, 4],
        default_left=True,
        left_child_key=20,
        right_child_key=10,
    )
    builder[0][20].set_leaf_node(2.0)
    builder[0][5].set_root()

    model = builder.commit()
    if test_round_trip:
        checkpoint_path = os.path.join(tmpdir, "checkpoint.bin")
        model.serialize(checkpoint_path)
        model = treelite.Model.deserialize(checkpoint_path)
    assert model.num_feature == num_feature
    assert model.num_tree == 1

    libpath = pathlib.Path(tmpdir) / ("libtest" + _libext())
    tl2cgen.export_lib(model, toolchain=toolchain, libpath=libpath, verbose=True)
    predictor = tl2cgen.Predictor(libpath=libpath)
    assert predictor.num_feature == num_feature
    assert predictor.num_target == 1
    np.testing.assert_array_equal(predictor.num_class, np.array([1], dtype=np.int32))
    for f0 in [-0.5, 0.5, 1.5, np.nan]:
        for f1 in [0, 1, 2, 3, 4, np.nan]:
            for f2 in [-1.0, -0.5, 1.0, np.nan]:
                x = np.array([[f0, f1, f2]])
                dmat = tl2cgen.DMatrix(x, dtype="float32")
                pred = predictor.predict(dmat)
                if f1 in [1, 2, 4] or np.isnan(f1):
                    expected_pred = 2.0
                elif f0 <= 0.5 and not np.isnan(f0):
                    expected_pred = 0.0
                else:
                    expected_pred = 1.0
                if pred != expected_pred:
                    raise ValueError(
                        f"Prediction wrong for f0={f0}, f1={f1}, f2={f2}: "
                        + f"expected_pred = {expected_pred} vs actual_pred = {pred}"
                    )
