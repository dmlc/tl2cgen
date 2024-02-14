# -*- coding: utf-8 -*-
"""Utility functions for tests"""
import os
import tempfile
from contextlib import contextmanager
from sys import platform as _platform
from typing import Iterator, List

import numpy as np
import treelite
from sklearn.datasets import load_svmlight_file

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
    tl_model = load_example_model(dataset)

    # GTIL doesn't yet support sparse matrix; so use NaN to represent missing values
    X_test = load_svmlight_file(example_model_db[dataset].dtest, zero_based=True)[
        0
    ].toarray()
    X_test[X_test == 0] = "nan"

    expected_margin = treelite.gtil.predict(tl_model, X_test, pred_margin=True)
    if len(expected_margin.shape) == 3:
        expected_margin = np.transpose(expected_margin, axes=(1, 0, 2))
    np.testing.assert_almost_equal(out_margin, expected_margin, decimal=5)

    expected_prob = treelite.gtil.predict(tl_model, X_test)
    if len(expected_prob.shape) == 3:
        expected_prob = np.transpose(expected_prob, axes=(1, 0, 2))
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
