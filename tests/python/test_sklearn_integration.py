"""Tests for scikit-learn importer"""

import pathlib

import numpy as np
import pytest
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

try:
    from hypothesis import assume, given, settings
    from hypothesis.strategies import data as hypothesis_callback
    from hypothesis.strategies import floats, integers, just, sampled_from
except ImportError:
    pytest.skip("hypothesis not installed; skipping", allow_module_level=True)


try:
    from sklearn import __version__ as sklearn_version
    from sklearn.datasets import make_classification
    from sklearn.dummy import DummyClassifier, DummyRegressor
    from sklearn.ensemble import (
        ExtraTreesClassifier,
        ExtraTreesRegressor,
        GradientBoostingClassifier,
        GradientBoostingRegressor,
        HistGradientBoostingClassifier,
        HistGradientBoostingRegressor,
        IsolationForest,
        RandomForestClassifier,
        RandomForestRegressor,
    )
    from sklearn.utils import shuffle
except ImportError:
    pytest.skip("scikit-learn not installed; skipping", allow_module_level=True)


try:
    import pandas as pd
except ImportError:
    pytest.skip("Pandas not installed; skipping", allow_module_level=True)


from .hypothesis_util import (
    standard_classification_datasets,
    standard_regression_datasets,
    standard_settings,
)
from .util import TemporaryDirectory, os_compatible_toolchains, to_categorical


@given(
    clazz=sampled_from(
        [
            RandomForestRegressor,
            ExtraTreesRegressor,
            GradientBoostingRegressor,
            HistGradientBoostingRegressor,
        ]
    ),
    n_estimators=integers(min_value=5, max_value=10),
    toolchain=sampled_from(os_compatible_toolchains()),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_skl_regressor(clazz, n_estimators, toolchain, callback):
    """Scikit-learn regressor"""
    if clazz in [RandomForestRegressor, ExtraTreesRegressor]:
        n_targets = callback.draw(integers(min_value=1, max_value=3))
    else:
        n_targets = callback.draw(just(1))
    X, y = callback.draw(standard_regression_datasets(n_targets=just(n_targets)))
    kwargs = {"max_depth": 8, "random_state": 0}
    if clazz == HistGradientBoostingRegressor:
        kwargs["max_iter"] = n_estimators
    else:
        kwargs["n_estimators"] = n_estimators
    if clazz in [GradientBoostingRegressor, HistGradientBoostingRegressor]:
        kwargs["learning_rate"] = callback.draw(floats(min_value=0.01, max_value=1.0))
    else:
        kwargs["n_jobs"] = -1
    if clazz == GradientBoostingClassifier:
        kwargs["init"] = callback.draw(
            sampled_from([None, DummyRegressor(strategy="mean"), "zero"])
        )
    clf = clazz(**kwargs)
    clf.fit(X, y)
    tl_model = treelite.sklearn.import_model(clf)
    expected_pred = treelite.gtil.predict(tl_model, X)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("predictor" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"quantize": 1, "parallel_comp": tl_model.num_tree},
        )
        predictor = tl2cgen.Predictor(libpath)
        out_pred = predictor.predict(tl2cgen.DMatrix(X))
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=3)


@given(
    clazz=sampled_from(
        [
            RandomForestClassifier,
            ExtraTreesClassifier,
            GradientBoostingClassifier,
            HistGradientBoostingClassifier,
        ]
    ),
    dataset=standard_classification_datasets(
        n_classes=integers(min_value=2, max_value=5),
        n_informative=just(5),
    ),
    n_estimators=integers(min_value=3, max_value=10),
    toolchain=sampled_from(os_compatible_toolchains()),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_skl_classifier(clazz, dataset, n_estimators, toolchain, callback):
    """Scikit-learn classifier"""
    X, y = dataset
    kwargs = {"max_depth": 8, "random_state": 0}
    if clazz == HistGradientBoostingClassifier:
        kwargs["max_iter"] = n_estimators
    else:
        kwargs["n_estimators"] = n_estimators
    if clazz in [GradientBoostingClassifier, HistGradientBoostingClassifier]:
        kwargs["learning_rate"] = callback.draw(floats(min_value=0.01, max_value=1.0))
    else:
        kwargs["n_jobs"] = -1
    if clazz == GradientBoostingClassifier:
        kwargs["init"] = callback.draw(
            sampled_from([None, DummyClassifier(strategy="prior"), "zero"])
        )
    clf = clazz(**kwargs)
    clf.fit(X, y)
    tl_model = treelite.sklearn.import_model(clf)
    expected_prob = treelite.gtil.predict(tl_model, X)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("predictor" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"quantize": 1, "parallel_comp": tl_model.num_tree},
        )
        predictor = tl2cgen.Predictor(libpath)
        out_prob = predictor.predict(tl2cgen.DMatrix(X))
        np.testing.assert_almost_equal(out_prob, expected_prob, decimal=5)


@given(
    n_classes=integers(min_value=3, max_value=5),
    n_estimators=integers(min_value=3, max_value=10),
    toolchain=sampled_from(os_compatible_toolchains()),
)
@settings(**standard_settings())
def test_skl_multitarget_multiclass_rf(n_classes, n_estimators, toolchain):
    """Scikit-learn RF classifier with multiple outputs and classes"""
    X, y1 = make_classification(
        n_samples=1000,
        n_features=100,
        n_informative=30,
        n_classes=n_classes,
        random_state=0,
    )
    y2 = shuffle(y1, random_state=1)
    y3 = shuffle(y1, random_state=2)

    y = np.vstack((y1, y2, y3)).T

    clf = RandomForestClassifier(
        max_depth=8, n_estimators=n_estimators, n_jobs=-1, random_state=4
    )
    clf.fit(X, y)
    tl_model = treelite.sklearn.import_model(clf)
    expected_prob = treelite.gtil.predict(tl_model, X)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("predictor" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"quantize": 1, "parallel_comp": tl_model.num_tree},
        )
        predictor = tl2cgen.Predictor(libpath)
        out_prob = predictor.predict(tl2cgen.DMatrix(X))
        np.testing.assert_almost_equal(out_prob, expected_prob, decimal=5)


@given(
    dataset=standard_regression_datasets(),
    toolchain=sampled_from(os_compatible_toolchains()),
)
@settings(**standard_settings())
def test_skl_converter_iforest(dataset, toolchain):
    """Scikit-learn isolation forest"""
    X, _ = dataset
    clf = IsolationForest(
        max_samples=64,
        n_estimators=10,
        n_jobs=-1,
        random_state=0,
    )
    clf.fit(X)
    tl_model = treelite.sklearn.import_model(clf)
    expected_pred = treelite.gtil.predict(tl_model, X)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("predictor" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"quantize": 1, "parallel_comp": tl_model.num_tree},
        )
        predictor = tl2cgen.Predictor(libpath)
        out_pred = predictor.predict(tl2cgen.DMatrix(X))
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=2)


@given(
    dataset=standard_classification_datasets(
        n_classes=integers(min_value=2, max_value=4),
    ),
    num_boost_round=integers(min_value=5, max_value=20),
    use_categorical=sampled_from([True, False]),
    toolchain=sampled_from(os_compatible_toolchains()),
    callback=hypothesis_callback(),
)
@settings(**standard_settings())
def test_skl_hist_gradient_boosting_with_categorical(
    dataset, num_boost_round, use_categorical, toolchain, callback
):
    # pylint: disable=too-many-locals
    """Scikit-learn HistGradientBoostingClassifier, with categorical splits"""
    X, y = dataset
    if use_categorical:
        n_categorical = callback.draw(sampled_from([3, 5, X.shape[1]]))
        assume(n_categorical <= X.shape[1])
        df, X_pred = to_categorical(X, n_categorical=n_categorical, invalid_frac=0.0)
        cat_col_bitmap = df.dtypes == "category"
        categorical_features = [
            i for i in range(len(cat_col_bitmap)) if cat_col_bitmap[i]
        ]
    else:
        df = pd.DataFrame(X)
        categorical_features = None
        X_pred = X.copy()

    clf = HistGradientBoostingClassifier(
        max_iter=num_boost_round,
        categorical_features=categorical_features,
        early_stopping=False,
        max_depth=8,
    )
    clf.fit(df, y)

    tl_model = treelite.sklearn.import_model(clf)
    expected_pred = treelite.gtil.predict(tl_model, X_pred)

    with TemporaryDirectory() as tmpdir:
        libpath = pathlib.Path(tmpdir) / ("predictor" + _libext())
        tl2cgen.export_lib(
            tl_model,
            toolchain=toolchain,
            libpath=libpath,
            params={"quantize": 1, "parallel_comp": tl_model.num_tree},
        )
        predictor = tl2cgen.Predictor(libpath)
        out_pred = predictor.predict(tl2cgen.DMatrix(X_pred))
        np.testing.assert_almost_equal(out_pred, expected_pred, decimal=4)
