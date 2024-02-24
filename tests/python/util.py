# -*- coding: utf-8 -*-
"""Utility functions for tests"""
import ctypes
import os
import tempfile
from contextlib import contextmanager
from math import ceil
from sys import platform as _platform
from typing import Any, Iterator, List, Optional, Tuple, Union

import numpy as np
import pytest
import treelite

import tl2cgen
from tl2cgen.contrib.util import _libext

from .metadata import example_model_db, load_example_model


def os_compatible_toolchains() -> List[str]:
    """Get the list of C compilers to test with the current OS"""
    if _platform == "darwin":
        gcc = os.environ.get("GCC_PATH", "gcc")
        toolchains = [gcc]
    elif _platform == "win32":
        toolchains = ["msvc"]
    else:
        toolchains = ["gcc", "clang"]
    return toolchains


def os_platform() -> str:
    """Detect OS that's running this program"""
    if _platform == "darwin":
        return "osx"
    if _platform in ["win32", "cygwin"]:
        return "windows"
    return "unix"


def libname(fmt: str) -> str:
    """Format name for a shared library, using appropriate file extension"""
    return fmt.format(_libext())


@contextmanager
def does_not_raise() -> Iterator[None]:
    """Placeholder to indicate that a section of code is not expected to raise any exception"""
    yield


def has_pandas():
    """Check whether pandas is available"""
    try:
        import pandas  # pylint: disable=unused-import,import-outside-toplevel

        return True
    except ImportError:
        return False


def check_predictor(predictor: tl2cgen.Predictor, dataset: str) -> None:
    """Check whether a predictor produces correct predictions for a given dataset"""
    try:
        from sklearn.datasets import load_svmlight_file
    except ImportError:
        pytest.skip("scikit-learn is required")

    dmat = tl2cgen.DMatrix(
        load_svmlight_file(example_model_db[dataset].dtest, zero_based=True)[0],
        dtype=example_model_db[dataset].dtype,
    )
    out_margin = predictor.predict(dmat, pred_margin=True)
    out_prob = predictor.predict(dmat)
    check_predictor_output(dataset, out_margin, out_prob)


def check_predictor_output(
    dataset: str, out_margin: np.ndarray, out_prob: np.ndarray
) -> None:
    """Check whether a predictor produces correct predictions"""
    try:
        from sklearn.datasets import load_svmlight_file
    except ImportError:
        pytest.skip("scikit-learn is required")

    tl_model = load_example_model(dataset)

    X_test = load_svmlight_file(example_model_db[dataset].dtest, zero_based=True)[0]
    expected_margin = treelite.gtil.predict(tl_model, X_test, pred_margin=True)
    np.testing.assert_almost_equal(out_margin, expected_margin, decimal=5)
    expected_prob = treelite.gtil.predict(tl_model, X_test)
    np.testing.assert_almost_equal(out_prob, expected_prob, decimal=5)


@contextmanager
def TemporaryDirectory(*args, **kwargs) -> Iterator[str]:
    # pylint: disable=C0103
    """
    Simulate the effect of 'ignore_cleanup_errors' parameter of tempfile.TemporaryDirectory.
    The parameter is only available for Python >= 3.10.
    """
    if "PYTEST_TMPDIR" in os.environ and "dir" not in kwargs:
        kwargs["dir"] = os.environ["PYTEST_TMPDIR"]
    tmpdir = tempfile.TemporaryDirectory(*args, **kwargs)
    try:
        yield tmpdir.name
    finally:
        try:
            tmpdir.cleanup()
        except (PermissionError, NotADirectoryError):
            if _platform != "win32":
                raise


# pylint: disable=R0914
def to_categorical(
    features: np.ndarray,
    *,
    n_categorical: int,
    invalid_frac: float = 0.0,
    random_state: Optional[Union[int, np.random.RandomState]] = None,
) -> Tuple[Any, np.ndarray]:
    """Generate a Pandas DataFrame with a mix of numerical and categorical columns.
    Also returns a NumPy array whose value is equal to the DataFrame, except for
    a given fraction of elements (they will be replaced with invalid categorical values).

    Credit: Levs Dolgovs (@levsnv)
    https://github.com/rapidsai/cuml/blob/3bab4d1/python/cuml/tests/test_fil.py#L641

    Returns
    -------
    generated_df:
        Generated DataFrame
    generated_array:
        NumPy array whose value is equal to generated_df, except with a fraction of elements
        replaced with an invalid categorical value.

    Parameters
    ----------
    features:
        A 2D NumPy array, to be used to generate the DataFrame. Use
        :py:meth:`~sklearn.datasets.make_regression` or
        :py:meth:`~sklearn.datasets.make_classification` to randomly generate this array.
    n_categorical:
        Number of categorical columns in the generated DataFrame.
    invalid_frac:
        Fraction of elements in the categorical columns that will be replaced with invalid
        categorical values. The replacement will occur only in generated_array, not generated_df.
    random_state:
        Random seed or RandomState object, to control the behavior of randomness
    """
    if not has_pandas():
        pytest.skip(reason="Pandas is required for to_categorical()")
    import pandas as pd

    features = features.copy()  # Avoid clobbering source matrix
    seed = ctypes.c_size_t(hash(random_state)).value  # Allow RandomState object
    rng = np.random.default_rng(seed=seed)
    n_features = features.shape[1]
    # First n_categorical columns will be converted into categorical columns.
    cat_cols = features[:, :n_categorical]
    # Normalize the columns to fit range [0, 1]
    cat_cols = cat_cols - cat_cols.min(axis=0, keepdims=True)
    cat_cols /= cat_cols.max(axis=0, keepdims=True)
    # Bin the columns into 100 bins to obtain categorical values
    rough_n_categories = 100
    cat_cols = (cat_cols * rough_n_categories).astype(int)

    # Mix categorical and numerical columns in a random order
    new_col_idx = rng.choice(n_features, n_features, replace=False, shuffle=True)
    df_cols = {}
    for icol in range(n_categorical):
        col = cat_cols[:, icol]
        df_cols[new_col_idx[icol]] = pd.Series(
            pd.Categorical(col, categories=np.unique(col))
        )
    for icol in range(n_categorical, n_features):
        df_cols[new_col_idx[icol]] = pd.Series(features[:, icol])
    generated_df = pd.DataFrame(df_cols)
    generated_df.sort_index(axis=1, inplace=True)

    # Randomly inject invalid categories
    invalid_idx = rng.choice(
        a=cat_cols.size,
        size=ceil(cat_cols.size * invalid_frac),
        replace=False,
        shuffle=False,
    )
    cat_cols.flat[invalid_idx] += 1000
    generated_array = np.concatenate([cat_cols, features[:, n_categorical:]], axis=1)
    generated_array[:, new_col_idx] = generated_array

    return generated_df, generated_array
