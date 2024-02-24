"""Tests for LightGBM integration"""

from __future__ import annotations

import pathlib
from typing import Any, Dict, Optional, Union

import numpy as np
import pytest
import scipy.sparse
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

try:
    import lightgbm
except ImportError:
    pytest.skip("LightGBM not installed; skipping", allow_module_level=True)

try:
    from hypothesis import given, settings
    from hypothesis.strategies import data as hypothesis_callback
    from hypothesis.strategies import integers, just, sampled_from
except ImportError:
    pytest.skip("hypothesis not installed; skipping", allow_module_level=True)


try:
    from sklearn.model_selection import train_test_split
except ImportError:
    pytest.skip("scikit-learn not installed; skipping", allow_module_level=True)


from .hypothesis_util import (
    standard_classification_datasets,
    standard_regression_datasets,
    standard_settings,
)
from .metadata import format_libpath_for_example_model, load_example_model
from .util import (
    TemporaryDirectory,
    check_predictor,
    has_pandas,
    os_compatible_toolchains,
    os_platform,
    to_categorical,
)


def _compile_lightgbm_model(  # pylint: disable=too-many-arguments
    *,
    model_path: Optional[pathlib.Path] = None,
    model_obj: Optional[lightgbm.Booster] = None,
    libname: str,
    prefix: Union[pathlib.Path, str],
    toolchain: str,
    params: Dict[str, Any],
    verbose: bool = False,
) -> tl2cgen.Predictor:
    if model_path is not None:
        model = treelite.Model.load(str(model_path), model_format="lightgbm")
    elif model_obj is not None:
        model = treelite.Model.from_lightgbm(model_obj)
    else:
        raise RuntimeError("Either model_path or model_obj must be provided")
    libpath = pathlib.Path(prefix).expanduser().resolve().joinpath(libname + _libext())
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params=params, verbose=verbose
    )
    return tl2cgen.Predictor(libpath=libpath, verbose=True)


@given(
    toolchain=sampled_from(os_compatible_toolchains()),
    objective=sampled_from(["regression", "regression_l1", "huber"]),
    reg_sqrt=sampled_from([True, False]),
    dataset=standard_regression_datasets(),
)
@settings(**standard_settings())
def test_lightgbm_regression(toolchain, objective, reg_sqrt, dataset):
    # pylint: disable=too-many-locals
    """Test a regressor"""
    X, y = dataset
    X_train, X_test, y_train, y_test = train_test_split(
        X, y, test_size=0.2, shuffle=False
    )
    dtrain = lightgbm.Dataset(X_train, y_train, free_raw_data=False)
    dtest = lightgbm.Dataset(X_test, y_test, reference=dtrain, free_raw_data=False)
    param = {
        "task": "train",
        "boosting_type": "gbdt",
        "objective": objective,
        "reg_sqrt": reg_sqrt,
        "metric": "rmse",
        "num_leaves": 31,
        "learning_rate": 0.05,
    }
    bst = lightgbm.train(
        param,
        dtrain,
        num_boost_round=10,
        valid_sets=[dtrain, dtest],
        valid_names=["train", "test"],
    )

    with TemporaryDirectory() as tmpdir:
        predictor = _compile_lightgbm_model(
            model_obj=bst,
            libname=f"regression_{objective}",
            prefix=tmpdir,
            toolchain=toolchain,
            params={"quantize": 1},
            verbose=True,
        )
        dmat = tl2cgen.DMatrix(X_test, dtype="float64")
        out_pred = predictor.predict(dmat)
        expected_pred = bst.predict(X_test).reshape((X_test.shape[0], 1, -1))
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=4)


@given(
    dataset=standard_classification_datasets(
        n_classes=integers(min_value=3, max_value=5), n_informative=just(5)
    ),
    objective=sampled_from(["multiclass", "multiclassova"]),
    boosting_type=sampled_from(["gbdt", "rf"]),
    num_boost_round=integers(min_value=5, max_value=50),
    use_categorical=sampled_from([True, False]),
    toolchain=sampled_from(os_compatible_toolchains()),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_lightgbm_multiclass_classification(
    dataset,
    objective,
    boosting_type,
    num_boost_round,
    use_categorical,
    toolchain,
    callback,
):
    # pylint: disable=too-many-locals,too-many-arguments
    """Test a multi-class classifier"""
    X, y = dataset
    num_class = np.max(y) + 1
    if use_categorical:
        n_categorical = callback.draw(integers(min_value=1, max_value=X.shape[1]))
        df, X_pred = to_categorical(X, n_categorical=n_categorical, invalid_frac=0.1)
        dtrain = lightgbm.Dataset(
            df, label=y, free_raw_data=False, categorical_feature="auto"
        )
    else:
        dtrain = lightgbm.Dataset(X, label=y, free_raw_data=False)
        X_pred = X.copy()
    param = {
        "task": "train",
        "boosting": boosting_type,
        "objective": objective,
        "metric": "multi_logloss",
        "num_class": num_class,
        "num_leaves": 31,
        "learning_rate": 0.05,
    }
    if boosting_type == "rf":
        param.update({"bagging_fraction": 0.8, "bagging_freq": 1})
    bst = lightgbm.train(
        param,
        dtrain,
        num_boost_round=num_boost_round,
        valid_sets=[dtrain],
        valid_names=["train"],
    )
    tl_model = treelite.frontend.from_lightgbm(bst)
    expected_prob = treelite.gtil.predict(tl_model, X_pred)
    expected_margin = treelite.gtil.predict(tl_model, X_pred, pred_margin=True)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / (f"lgb_multiclass_{objective}" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"parallel_comp": tl_model.num_tree, "quantize": 1},
        )
        predictor = tl2cgen.Predictor(libpath)
        dmat = tl2cgen.DMatrix(X_pred, dtype="float64")
        out_prob = predictor.predict(dmat)
        out_margin = predictor.predict(dmat, pred_margin=True)
        np.testing.assert_almost_equal(out_prob, expected_prob, decimal=5)
        np.testing.assert_almost_equal(out_margin, expected_margin, decimal=5)


@given(
    dataset=standard_classification_datasets(n_classes=just(2)),
    objective=sampled_from(["binary", "xentlambda", "xentropy"]),
    num_boost_round=integers(min_value=5, max_value=10),
    use_categorical=sampled_from([True, False]),
    toolchain=sampled_from(os_compatible_toolchains()),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_lightgbm_binary_classification(
    dataset,
    objective,
    num_boost_round,
    use_categorical,
    toolchain,
    callback,
):
    # pylint: disable=too-many-locals,too-many-arguments
    """Test a binary classifier"""
    X, y = dataset
    if use_categorical:
        n_categorical = callback.draw(integers(min_value=1, max_value=X.shape[1]))
        df, X_pred = to_categorical(X, n_categorical=n_categorical, invalid_frac=0.1)
        dtrain = lightgbm.Dataset(
            df, label=y, free_raw_data=False, categorical_feature="auto"
        )
    else:
        dtrain = lightgbm.Dataset(X, label=y, free_raw_data=False)
        X_pred = X.copy()

    params = {
        "task": "train",
        "boosting_type": "gbdt",
        "objective": objective,
        "metric": "auc",
        "num_leaves": 7,
        "learning_rate": 0.1,
    }

    bst = lightgbm.train(
        params,
        dtrain,
        num_boost_round=num_boost_round,
        valid_sets=[dtrain],
        valid_names=["train"],
    )
    with TemporaryDirectory() as tmpdir:
        model_path = pathlib.Path(tmpdir) / f"lgb_binary_{objective}.txt"
        bst.save_model(model_path)
        tl_model = treelite.frontend.load_lightgbm_model(model_path)
        expected_prob = treelite.gtil.predict(tl_model, X_pred)
        expected_margin = treelite.gtil.predict(tl_model, X_pred, pred_margin=True)

        libpath = pathlib.Path(tmpdir) / (f"lgb_binary_{objective}" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"parallel_comp": tl_model.num_tree},
            verbose=True,
        )
        predictor = tl2cgen.Predictor(libpath)
        dtest = tl2cgen.DMatrix(X_pred)
        out_prob = predictor.predict(dtest)
        out_margin = predictor.predict(dtest, pred_margin=True)
        np.testing.assert_almost_equal(out_prob, expected_prob, decimal=5)
        np.testing.assert_almost_equal(out_margin, expected_margin, decimal=5)


@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
@pytest.mark.parametrize("parallel_comp", [None, 2])
@pytest.mark.parametrize("quantize", [True, False])
def test_categorical_data(tmpdir, quantize, parallel_comp, toolchain):
    """
    LightGBM is able to produce categorical splits directly, so that
    categorical data don't have to be one-hot encoded. Test if Treelite is
    able to handle categorical splits.

    This toy example contains two features, both of which are categorical.
    The first has cardinality 3 and the second 5. The label was generated using
    the formula

       y = f(x0) + g(x1) + [noise with std=0.1]

    where f and g are given by the tables

       x0  f(x0)        x1  g(x1)
        0    -20         0     -2
        1    -10         1     -1
        2      0         2      0
                         3      1
                         4      2
    """
    dataset = "toy_categorical"
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)

    # pylint: disable=R0801
    params = {
        "quantize": (1 if quantize else 0),
        "parallel_comp": (parallel_comp if parallel_comp else 0),
    }
    tl2cgen.export_lib(
        model, toolchain=toolchain, libpath=libpath, params=params, verbose=True
    )
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    check_predictor(predictor, dataset)


@pytest.mark.skipif(not has_pandas(), reason="Pandas required")
@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_categorical_data_pandas_df_with_dummies(tmpdir, toolchain):
    # pylint: disable=too-many-locals
    """
    This toy example contains two features, both of which are categorical.
    The first has cardinality 3 and the second 5. The label was generated using
    the formula

       y = f(x0) + g(x1) + [noise with std=0.1]

    where f and g are given by the tables

       x0  f(x0)        x1  g(x1)
        0    -20         0     -2
        1    -10         1     -1
        2      0         2      0
                         3      1
                         4      2

    Unlike test_categorical_data(), this test will convert the categorical features into
    dummies using OneHotEncoder and store them into a Pandas Dataframe.
    """
    import pandas as pd  # pylint: disable=import-outside-toplevel

    rng = np.random.default_rng(seed=0)
    n_row = 100
    x0 = rng.integers(low=0, high=3, size=n_row)
    x1 = rng.integers(low=0, high=5, size=n_row)
    noise = rng.standard_normal(size=n_row) * 0.1
    y = (x0 * 10 - 20) + (x1 - 2) + noise
    df = pd.DataFrame({"x0": x0, "x1": x1}).astype("category")
    df_dummies = pd.get_dummies(df, prefix=["x0", "x1"], prefix_sep="=")
    assert df_dummies.columns.tolist() == [
        "x0=0",
        "x0=1",
        "x0=2",
        "x1=0",
        "x1=1",
        "x1=2",
        "x1=3",
        "x1=4",
    ]

    model_path = pathlib.Path(tmpdir) / "toy_dummies.txt"

    dtrain = lightgbm.Dataset(df_dummies, label=y)
    param = {
        "task": "train",
        "boosting_type": "gbdt",
        "objective": "regression",
        "metric": "rmse",
        "num_leaves": 7,
        "learning_rate": 0.1,
    }
    bst = lightgbm.train(
        param, dtrain, num_boost_round=15, valid_sets=[dtrain], valid_names=["train"]
    )
    lgb_out = bst.predict(df_dummies).reshape((df_dummies.shape[0], 1, -1))
    bst.save_model(model_path)

    predictor = _compile_lightgbm_model(
        model_path=model_path,
        libname="dummies",
        prefix=tmpdir,
        toolchain=toolchain,
        params={},
        verbose=True,
    )
    tl_out = predictor.predict(tl2cgen.DMatrix(df_dummies.values, dtype="float32"))
    np.testing.assert_almost_equal(lgb_out, tl_out, decimal=5)


@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
@pytest.mark.parametrize("quantize", [True, False])
def test_sparse_ranking_model(tmpdir, quantize, toolchain):
    # pylint: disable=too-many-locals
    """Generate a LightGBM ranking model with highly sparse data.
    This example is inspired by https://github.com/dmlc/treelite/issues/222. It verifies that
    Treelite is able to accommodate the unique behavior of LightGBM when it comes to handling
    missing values.

    LightGBM offers two modes of handling missing values:

    1. Assign default direction per each test node: This is similar to how XGBoost handles missing
       values.
    2. Replace missing values with zeroes (0.0): This behavior is unique to LightGBM.

    The mode is controlled by the missing_value_to_zero_ field of each test node.

    This example is crafted so as to invoke the second mode of missing value handling.
    """
    rng = np.random.default_rng(seed=2020)
    X = scipy.sparse.random(
        m=10, n=206947, format="csr", dtype=np.float64, random_state=0, density=0.0001
    )
    X.data = rng.standard_normal(size=X.data.shape[0], dtype=np.float64)
    y = rng.integers(low=0, high=5, size=X.shape[0])

    params = {
        "objective": "lambdarank",
        "num_leaves": 32,
        "lambda_l1": 0.0,
        "lambda_l2": 0.0,
        "min_gain_to_split": 0.0,
        "learning_rate": 1.0,
        "min_data_in_leaf": 1,
    }

    model_path = pathlib.Path(tmpdir) / "sparse_ranking_lightgbm.txt"

    dtrain = lightgbm.Dataset(X, label=y, group=[X.shape[0]])

    bst = lightgbm.train(params, dtrain, num_boost_round=1)
    lgb_out = bst.predict(X).reshape((X.shape[0], 1, -1))
    bst.save_model(model_path)

    predictor = _compile_lightgbm_model(
        model_path=model_path,
        libname="sparse_ranking_lgb",
        prefix=tmpdir,
        toolchain=toolchain,
        params={"quantize": (1 if quantize else 0)},
        verbose=True,
    )
    dmat = tl2cgen.DMatrix(X)
    out = predictor.predict(dmat)
    np.testing.assert_almost_equal(out, lgb_out)


@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
@pytest.mark.parametrize("quantize", [True, False])
def test_sparse_categorical_model(tmpdir, quantize, toolchain):
    """
    LightGBM is able to produce categorical splits directly, so that
    categorical data don't have to be one-hot encoded. Test if Treelite is
    able to handle categorical splits.

    This example produces a model with high-cardinality categorical variables.
    The training data has many missing values, so we need to match LightGBM
    when it comes to handling missing values
    """
    if toolchain == "clang":
        pytest.xfail(reason="Clang cannot handle long if conditional")
    if os_platform() == "windows":
        pytest.xfail(reason="MSVC cannot handle long if conditional")
    if os_platform() == "osx":
        pytest.xfail(reason="Apple Clang cannot handle long if conditional")
    dataset = "sparse_categorical"
    libpath = format_libpath_for_example_model(dataset, prefix=tmpdir)
    model = load_example_model(dataset)
    params = {"quantize": (1 if quantize else 0)}
    tl2cgen.export_lib(
        model,
        toolchain=toolchain,
        libpath=libpath,
        params=params,
        verbose=True,
        options=["-O0"],
    )
    predictor = tl2cgen.Predictor(libpath=libpath, verbose=True)
    check_predictor(predictor, dataset)


@pytest.mark.parametrize("toolchain", os_compatible_toolchains())
def test_nan_handling_with_categorical_splits(tmpdir, toolchain):
    """Test that NaN inputs are handled correctly in categorical splits"""

    # Test case taken from https://github.com/dmlc/treelite/issues/277
    X = np.array(30 * [[1]] + 30 * [[2]] + 30 * [[0]])
    y = np.array(60 * [5] + 30 * [10])
    train_data = lightgbm.Dataset(X, label=y, categorical_feature=[0])
    bst = lightgbm.train({}, train_data, 1)

    model_path = pathlib.Path(tmpdir) / "dummy_categorical.txt"

    input_with_nan = np.array([[np.NaN], [0.0]])
    lgb_pred = bst.predict(input_with_nan).reshape((input_with_nan.shape[0], 1, -1))
    bst.save_model(model_path)

    predictor = _compile_lightgbm_model(
        model_path=model_path,
        libname="dummy_categorical_lgb",
        prefix=tmpdir,
        toolchain=toolchain,
        params={},
        verbose=True,
    )
    dmat = tl2cgen.DMatrix(input_with_nan)
    tl_pred = predictor.predict(dmat)
    np.testing.assert_almost_equal(tl_pred, lgb_pred)
