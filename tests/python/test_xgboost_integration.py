"""Tests for XGBoost integration"""

# pylint: disable=R0201, R0915
import os

import numpy as np
import pytest
import treelite
from hypothesis import assume, given, settings
from hypothesis.strategies import integers, sampled_from
from sklearn.datasets import load_iris
from sklearn.model_selection import train_test_split

import tl2cgen
from tl2cgen.contrib.util import _libext

from .hypothesis_util import standard_regression_datasets, standard_settings
from .metadata import format_libpath_for_example_model, load_example_model
from .util import TemporaryDirectory, check_predictor, os_compatible_toolchains

try:
    import xgboost as xgb
except ImportError:
    # skip this test suite if XGBoost is not installed
    pytest.skip("XGBoost not installed; skipping", allow_module_level=True)


@given(
    toolchain=sampled_from(os_compatible_toolchains()),
    objective=sampled_from(
        [
            "reg:linear",
            "reg:squarederror",
            "reg:squaredlogerror",
            "reg:pseudohubererror",
        ]
    ),
    model_format=sampled_from(["binary", "json"]),
    num_parallel_tree=integers(min_value=1, max_value=10),
    dataset=standard_regression_datasets(),
)
@settings(**standard_settings())
def test_xgb_regression(toolchain, objective, model_format, num_parallel_tree, dataset):
    # pylint: disable=too-many-locals
    """Test a random regression dataset"""
    X, y = dataset
    if objective == "reg:squaredlogerror":
        assume(np.all(y > -1))
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, shuffle=False
    )
    dtrain = xgb.DMatrix(X_train, label=y_train)
    dtest = xgb.DMatrix(X_test, label=y_test)
    param = {
        "max_depth": 8,
        "eta": 1,
        "verbosity": 0,
        "objective": objective,
        "num_parallel_tree": num_parallel_tree,
    }
    num_round = 10
    bst = xgb.train(
        param,
        dtrain,
        num_boost_round=num_round,
        evals=[(dtrain, "train"), (dtest, "test")],
    )
    with TemporaryDirectory() as tmpdir:
        if model_format == "json":
            model_name = "regression.json"
            model_path = os.path.join(tmpdir, model_name)
            bst.save_model(model_path)
            model = treelite.frontend.load_xgboost_model(filename=model_path)
        else:
            model_name = "regression.model"
            model_path = os.path.join(tmpdir, model_name)
            bst.save_model(model_path)
            model = treelite.frontend.load_xgboost_model_legacy_binary(
                filename=model_path
            )

        assert model.num_feature == dtrain.num_col()
        assert model.num_tree == num_round * num_parallel_tree
        libpath = os.path.join(tmpdir, "regression" + _libext())
        tl2cgen.export_lib(
            model,
            toolchain=toolchain,
            libpath=libpath,
            params={"parallel_comp": model.num_tree},
            verbose=True,
        )

        predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
        assert predictor.num_feature == dtrain.num_col()
        assert predictor.num_target == 1
        np.testing.assert_array_equal(predictor.num_class, [1])
        dmat = tl2cgen.DMatrix(X_test, dtype="float32")
        out_pred = predictor.predict(dmat)
        expected_pred = bst.predict(dtest, strict_shape=True)[:, :, np.newaxis]
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=3)


@pytest.mark.parametrize("num_parallel_tree", [1, 3, 5])
@pytest.mark.parametrize("model_format", ["binary", "json"])
@pytest.mark.parametrize(
    "objective",
    ["multi:softmax", "multi:softprob"],
)
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_xgb_iris(
    tmpdir,
    toolchain,
    objective,
    model_format,
    num_parallel_tree,
):
    # pylint: disable=too-many-locals, too-many-arguments
    """Test Iris data (multi-class classification)"""
    X, y = load_iris(return_X_y=True)
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, shuffle=False
    )
    dtrain = xgb.DMatrix(X_train, label=y_train)
    dtest = xgb.DMatrix(X_test, label=y_test)
    num_class = 3
    num_round = 10
    param = {
        "max_depth": 6,
        "eta": 0.05,
        "num_class": num_class,
        "verbosity": 0,
        "objective": objective,
        "metric": "mlogloss",
        "num_parallel_tree": num_parallel_tree,
    }
    bst = xgb.train(
        param,
        dtrain,
        num_boost_round=num_round,
        evals=[(dtrain, "train"), (dtest, "test")],
    )

    if model_format == "json":
        model_name = "iris.json"
        model_path = os.path.join(tmpdir, model_name)
        bst.save_model(model_path)
        model = treelite.frontend.load_xgboost_model(filename=model_path)
    else:
        model_name = "iris.model"
        model_path = os.path.join(tmpdir, model_name)
        bst.save_model(model_path)
        model = treelite.frontend.load_xgboost_model_legacy_binary(filename=model_path)
    assert model.num_feature == dtrain.num_col()
    assert model.num_tree == num_round * num_class * num_parallel_tree
    libpath = os.path.join(tmpdir, "iris" + _libext())
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params={}, verbose=True
    )

    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == dtrain.num_col()
    assert predictor.num_target == 1
    np.testing.assert_array_equal(predictor.num_class, [num_class])
    dmat = tl2cgen.DMatrix(X_test, dtype="float32")
    out_pred = predictor.predict(dmat)

    if objective == "multi:softmax":
        out_pred = np.argmax(out_pred, axis=2)
        expected_pred = bst.predict(dtest, strict_shape=True)
    else:
        expected_pred = bst.predict(dtest, strict_shape=True)[:, np.newaxis, :]
    np.testing.assert_almost_equal(out_pred, expected_pred, decimal=5)


@pytest.mark.parametrize("model_format", ["binary", "json"])
@pytest.mark.parametrize(
    "objective,max_label",
    [
        ("binary:logistic", 2),
        ("binary:hinge", 2),
        ("binary:logitraw", 2),
        ("count:poisson", 4),
        ("rank:pairwise", 5),
        ("rank:ndcg", 5),
        ("rank:map", 2),
    ],
    ids=[
        "binary:logistic",
        "binary:hinge",
        "binary:logitraw",
        "count:poisson",
        "rank:pairwise",
        "rank:ndcg",
        "rank:map",
    ],
)
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_nonlinear_objective(tmpdir, objective, max_label, toolchain, model_format):
    # pylint: disable=too-many-locals,too-many-arguments
    """Test non-linear objectives with dummy data"""
    np.random.seed(0)
    nrow = 16
    ncol = 8
    X = np.random.randn(nrow, ncol)
    y = np.random.randint(0, max_label, size=nrow)
    assert np.min(y) == 0
    assert np.max(y) == max_label - 1

    num_round = 4
    dtrain = xgb.DMatrix(X, label=y)
    if objective.startswith("rank:"):
        dtrain.set_group([nrow])
    bst = xgb.train(
        {"objective": objective, "base_score": 0.5, "seed": 0},
        dtrain=dtrain,
        num_boost_round=num_round,
    )

    objective_tag = objective.replace(":", "_")
    if model_format == "json":
        model_name = f"nonlinear_{objective_tag}.json"
    else:
        model_name = f"nonlinear_{objective_tag}.bin"
    model_path = os.path.join(tmpdir, model_name)
    bst.save_model(model_path)

    if model_format == "json":
        model = treelite.frontend.load_xgboost_model(model_path)
    else:
        model = treelite.frontend.load_xgboost_model_legacy_binary(model_path)
    assert model.num_feature == dtrain.num_col()
    assert model.num_tree == num_round
    libpath = os.path.join(tmpdir, objective_tag + _libext())
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params={}, verbose=True
    )

    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == dtrain.num_col()
    assert predictor.num_target == 1
    np.testing.assert_array_equal(predictor.num_class, [1])
    dmat = tl2cgen.DMatrix(X, dtype="float32")
    out_pred = predictor.predict(dmat)
    expected_pred = bst.predict(dtrain, strict_shape=True)[:, :, np.newaxis]
    np.testing.assert_almost_equal(out_pred, expected_pred, decimal=5)


@given(
    toolchain=sampled_from(os_compatible_toolchains()),
    dataset=standard_regression_datasets(),
)
@settings(**standard_settings())
def test_xgb_deserializers(toolchain, dataset):
    # pylint: disable=too-many-locals
    """Test a random regression dataset and test serializers"""
    X, y = dataset
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, shuffle=False
    )
    dtrain = xgb.DMatrix(X_train, label=y_train)
    dtest = xgb.DMatrix(X_test, label=y_test)
    param = {"max_depth": 8, "eta": 1, "silent": 1, "objective": "reg:linear"}
    num_round = 10
    bst = xgb.train(
        param,
        dtrain,
        num_boost_round=num_round,
        evals=[(dtrain, "train"), (dtest, "test")],
    )

    with TemporaryDirectory() as tmpdir:
        # Serialize xgboost model
        model_bin_path = os.path.join(tmpdir, "serialized.model")
        bst.save_model(model_bin_path)
        model_json_path = os.path.join(tmpdir, "serialized.json")
        bst.save_model(model_json_path)
        model_json_str = bst.save_raw(raw_format="json")

        # Construct Treelite models from xgboost serializations
        model_bin = treelite.frontend.load_xgboost_model_legacy_binary(model_bin_path)
        model_json = treelite.frontend.load_xgboost_model(model_json_path)
        model_json_str = treelite.Model.from_xgboost_json(model_json_str)

        # Compile models to libraries
        libext = _libext()
        model_bin_lib = os.path.join(tmpdir, f"bin{libext}")
        tl2cgen.export_lib(
            model_bin,
            toolchain=toolchain,
            libpath=model_bin_lib,
            params={"parallel_comp": model_bin.num_tree},
        )
        model_json_lib = os.path.join(tmpdir, f"json{libext}")
        tl2cgen.export_lib(
            model_json,
            toolchain=toolchain,
            libpath=model_json_lib,
            params={"parallel_comp": model_json.num_tree},
        )
        model_json_str_lib = os.path.join(tmpdir, f"json_str{libext}")
        tl2cgen.export_lib(
            model_json_str,
            toolchain=toolchain,
            libpath=model_json_str_lib,
            params={"parallel_comp": model_json_str.num_tree},
        )

        # Generate predictors from compiled libraries
        predictor_bin = tl2cgen.Predictor(model_bin_lib)
        assert predictor_bin.num_feature == dtrain.num_col()
        assert predictor_bin.num_target == 1
        np.testing.assert_array_equal(predictor_bin.num_class, [1])

        predictor_json = tl2cgen.Predictor(model_json_lib)
        assert predictor_json.num_feature == dtrain.num_col()
        assert predictor_json.num_target == 1
        np.testing.assert_array_equal(predictor_json.num_class, [1])

        predictor_json_str = tl2cgen.Predictor(model_json_str_lib)
        assert predictor_json_str.num_feature == dtrain.num_col()
        assert predictor_json_str.num_target == 1
        np.testing.assert_array_equal(predictor_json_str.num_class, [1])

        # Run inference with each predictor
        dmat = tl2cgen.DMatrix(X_test, dtype="float32")
        bin_pred = predictor_bin.predict(dmat)
        json_pred = predictor_json.predict(dmat)
        json_str_pred = predictor_json_str.predict(dmat)

        expected_pred = bst.predict(dtest, strict_shape=True)[:, np.newaxis, :]
        np.testing.assert_almost_equal(bin_pred, expected_pred, decimal=4)
        np.testing.assert_almost_equal(json_pred, expected_pred, decimal=4)
        np.testing.assert_almost_equal(json_str_pred, expected_pred, decimal=4)


@pytest.mark.parametrize("parallel_comp", [None, 5])
@pytest.mark.parametrize("quantize", [True, False])
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_xgb_categorical_split(tmpdir, toolchain, quantize, parallel_comp):
    """Test toy XGBoost model with categorical splits"""
    dataset = "xgb_toy_categorical"
    model = load_example_model(dataset)
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)

    params = {
        "quantize": (1 if quantize else 0),
        "parallel_comp": (parallel_comp if parallel_comp else 0),
    }
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params=params, verbose=True
    )

    predictor = tl2cgen.Predictor(libpath)
    check_predictor(predictor, dataset)


@pytest.mark.parametrize("model_format", ["binary", "json"])
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_xgb_dart(tmpdir, toolchain, model_format):
    # pylint: disable=too-many-locals,too-many-arguments
    """Test dart booster with dummy data"""
    np.random.seed(0)
    nrow = 16
    ncol = 8
    X = np.random.randn(nrow, ncol)
    y = np.random.randint(0, 2, size=nrow)
    assert np.min(y) == 0
    assert np.max(y) == 1

    num_round = 50
    dtrain = xgb.DMatrix(X, label=y)
    param = {
        "booster": "dart",
        "max_depth": 5,
        "learning_rate": 0.1,
        "objective": "binary:logistic",
        "sample_type": "uniform",
        "normalize_type": "tree",
        "rate_drop": 0.1,
        "skip_drop": 0.5,
    }
    bst = xgb.train(param, dtrain=dtrain, num_boost_round=num_round)

    if model_format == "json":
        model_json_path = os.path.join(tmpdir, "serialized.json")
        bst.save_model(model_json_path)
        model = treelite.frontend.load_xgboost_model(model_json_path)
    else:
        model_bin_path = os.path.join(tmpdir, "serialized.model")
        bst.save_model(model_bin_path)
        model = treelite.frontend.load_xgboost_model_legacy_binary(model_bin_path)

    assert model.num_feature == dtrain.num_col()
    assert model.num_tree == num_round
    libpath = os.path.join(tmpdir, "dart" + _libext())
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params={}, verbose=True
    )

    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    assert predictor.num_feature == dtrain.num_col()
    assert predictor.num_target == 1
    np.testing.assert_array_equal(predictor.num_class, [1])
    dmat = tl2cgen.DMatrix(X, dtype="float32")
    out_pred = predictor.predict(dmat)
    expected_pred = bst.predict(dtrain, strict_shape=True)[:, np.newaxis, :]
    np.testing.assert_almost_equal(out_pred, expected_pred, decimal=5)
