"""Test whether TL2cgen handles invalid category values correctly"""

import pathlib

import numpy as np
import pytest
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

from .util import os_compatible_toolchains


@pytest.fixture(name="toy_model")
def toy_model_fixture():
    """A toy model with a single tree containing a categorical split"""
    builder = treelite.ModelBuilder(num_feature=2)
    tree = treelite.ModelBuilder.Tree()
    tree[0].set_categorical_test_node(
        feature_id=1,
        left_categories=[0],
        default_left=True,
        left_child_key=1,
        right_child_key=2,
    )
    tree[1].set_leaf_node(-1.0)
    tree[2].set_leaf_node(1.0)
    tree[0].set_root()
    builder.append(tree)
    model = builder.commit()

    return model


@pytest.fixture(name="test_data")
def test_data_fixture():
    """Generate test data consisting of two columns"""
    categorical_column = np.array(
        [-1, -0.6, -0.5, 0, 0.3, 0.7, 1, np.nan, np.inf, 1e10, -1e10], dtype=np.float32
    )
    dummy_column = np.zeros(categorical_column.shape[0], dtype=np.float32)
    return np.column_stack((dummy_column, categorical_column))


@pytest.fixture(name="ref_pred")
def ref_pred_fixture():
    """Return correct output for the test data"""
    # Negative inputs are mapped to the right child node
    # 0.3 and 0.7 are mapped to the left child node, since they get rounded toward the zero.
    # Missing value gets mapped to the left child node, since default_left=True
    # inf, 1e10, and -1e10 don't match any element of left_categories, so they get mapped to the
    # right child.
    return np.array([1, 1, 1, -1, -1, -1, 1, -1, 1, 1, 1], dtype=np.float32).reshape(
        (-1, 1, 1)
    )


def test_invalid_categorical_input(tmpdir, toy_model, test_data, ref_pred):
    """Test whether TL2cgen handles invalid category values correctly"""
    libpath = pathlib.Path(tmpdir).joinpath("mylib" + _libext())
    toolchain = os_compatible_toolchains()[0]
    tl2cgen.export_lib(toy_model, toolchain=toolchain, libpath=libpath)

    predictor = tl2cgen.Predictor(libpath=libpath)
    dmat = tl2cgen.DMatrix(test_data)
    pred = predictor.predict(dmat)
    np.testing.assert_equal(pred, ref_pred)
