"""Tests for XGBoost integration"""

# pylint: disable=R0201, R0915
import os
import pathlib

import numpy as np
import pytest
import treelite
from hypothesis import assume, given, settings
from hypothesis.strategies import data as hypothesis_callback
from hypothesis.strategies import integers, just, sampled_from
from sklearn.model_selection import train_test_split

import tl2cgen
from tl2cgen.contrib.util import _libext

from .hypothesis_util import (
    standard_classification_datasets,
    standard_regression_datasets,
    standard_settings,
)
from .metadata import format_libpath_for_example_model, load_example_model
from .util import (
    TemporaryDirectory,
    check_predictor,
    os_compatible_toolchains,
    to_categorical,
)

try:
    import xgboost as xgb
except ImportError:
    # skip this test suite if XGBoost is not installed
    pytest.skip("XGBoost not installed; skipping", allow_module_level=True)


def generate_data_for_squared_log_error(n_targets: int = 1):
    """Generate data containing outliers."""
    n_rows = 4096
    n_cols = 16

    outlier_mean = 10000  # mean of generated outliers
    n_outliers = 64

    X = np.random.randn(n_rows, n_cols)
    y = np.random.randn(n_rows, n_targets)
    y += np.abs(np.min(y, axis=0))

    # Create outliers
    for _ in range(0, n_outliers):
        ind = np.random.randint(0, len(y) - 1)
        y[ind, :] += np.random.randint(0, outlier_mean)

    # rmsle requires all label be greater than -1.
    assert np.all(y > -1.0)

    return X, y


@given(
    toolchain=sampled_from(os_compatible_toolchains()),
    objective=sampled_from(
        [
            "reg:squarederror",
            "reg:squaredlogerror",
            "reg:pseudohubererror",
        ]
    ),
    num_boost_round=integers(min_value=3, max_value=10),
    num_parallel_tree=integers(min_value=1, max_value=3),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_xgb_regression(
    toolchain, objective, num_boost_round, num_parallel_tree, callback
):
    # pylint: disable=too-many-locals
    """Test a random regression dataset"""
    if objective == "reg:squaredlogerror":
        X, y = generate_data_for_squared_log_error()
        use_categorical = False
    else:
        X, y = callback.draw(standard_regression_datasets())
        use_categorical = callback.draw(sampled_from([True, False]))
    if use_categorical:
        n_categorical = callback.draw(sampled_from([3, 5, X.shape[1]]))
        assume(n_categorical <= X.shape[1])
        df, X_pred = to_categorical(X, n_categorical=n_categorical, invalid_frac=0.1)
        dtrain = xgb.DMatrix(df, label=y, enable_categorical=True)
        model_format = "json"
    else:
        dtrain = xgb.DMatrix(X, label=y)
        X_pred = X.copy()
        model_format = callback.draw(sampled_from(["json", "legacy_binary"]))
    param = {
        "max_depth": 8,
        "eta": 0.1,
        "verbosity": 0,
        "objective": objective,
        "num_parallel_tree": num_parallel_tree,
    }
    bst = xgb.train(
        param,
        dtrain,
        num_boost_round=num_boost_round,
        evals=[(dtrain, "train")],
    )
    with TemporaryDirectory() as tmpdir:
        if model_format == "json":
            model_path = pathlib.Path(tmpdir) / "regression.json"
            bst.save_model(model_path)
            model = treelite.frontend.load_xgboost_model(filename=model_path)
        else:
            model_path = pathlib.Path(tmpdir) / "regression.model"
            bst.save_model(model_path)
            model = treelite.frontend.load_xgboost_model_legacy_binary(
                filename=model_path
            )

        assert model.num_feature == dtrain.num_col()
        assert model.num_tree == num_boost_round * num_parallel_tree
        libpath = pathlib.Path(tmpdir) / ("regression" + _libext())
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
        dmat = tl2cgen.DMatrix(X_pred, dtype="float32")
        out_pred = predictor.predict(dmat)
        expected_pred = bst.predict(
            xgb.DMatrix(X_pred), strict_shape=True, validate_features=False
        )[:, :, np.newaxis]
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=3)


@given(
    dataset=standard_classification_datasets(
        n_classes=integers(min_value=3, max_value=5), n_informative=just(5)
    ),
    toolchain=sampled_from(os_compatible_toolchains()),
    objective=sampled_from(["multi:softmax", "multi:softprob"]),
    pred_margin=sampled_from([True, False]),
    num_boost_round=integers(min_value=3, max_value=5),
    num_parallel_tree=integers(min_value=1, max_value=2),
    use_categorical=sampled_from([True, False]),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_xgb_multiclass_classifier(
    dataset,
    toolchain,
    objective,
    pred_margin,
    num_boost_round,
    num_parallel_tree,
    use_categorical,
    callback,
):
    # pylint: disable=too-many-locals, too-many-arguments
    """Test XGBoost with multi-class classification problem"""
    X, y = dataset
    if use_categorical:
        n_categorical = callback.draw(integers(min_value=1, max_value=X.shape[1]))
        df, X_pred = to_categorical(X, n_categorical=n_categorical, invalid_frac=0.1)
        dtrain = xgb.DMatrix(df, label=y, enable_categorical=True)
    else:
        dtrain = xgb.DMatrix(X, label=y)
        X_pred = X.copy()
    num_class = np.max(y) + 1
    param = {
        "max_depth": 6,
        "eta": 0.1,
        "num_class": num_class,
        "verbosity": 0,
        "objective": objective,
        "metric": "mlogloss",
        "num_parallel_tree": num_parallel_tree,
    }
    bst = xgb.train(
        param,
        dtrain,
        num_boost_round=num_boost_round,
    )

    model = treelite.frontend.from_xgboost(bst)
    assert model.num_feature == dtrain.num_col()
    assert model.num_tree == num_boost_round * num_class * num_parallel_tree

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("iris" + _libext())
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
        np.testing.assert_array_equal(predictor.num_class, [num_class])
        dtest = tl2cgen.DMatrix(X_pred, dtype="float32")
        out_pred = predictor.predict(dtest, pred_margin=pred_margin)

        if objective == "multi:softmax" and not pred_margin:
            out_pred = np.argmax(out_pred, axis=2)
            expected_pred = bst.predict(
                xgb.DMatrix(X_pred), strict_shape=True, validate_features=False
            )
        else:
            expected_pred = bst.predict(
                xgb.DMatrix(X_pred),
                strict_shape=True,
                output_margin=pred_margin,
                validate_features=False,
            )[:, np.newaxis, :]

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
