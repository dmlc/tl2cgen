# -*- coding: utf-8 -*-
"""Utility functions for tests"""
import os
import tempfile
from contextlib import contextmanager
from sys import platform as _platform
from typing import Iterator, List, Optional, Tuple

import numpy as np
from sklearn.datasets import load_svmlight_file

import tl2cgen
from tl2cgen.contrib.util import _libext

from .metadata import example_model_db


def load_txt(filename: str) -> Optional[np.ndarray]:
    """Get 1D array from text file"""
    if filename is None:
        return None
    content = []
    with open(filename, "r", encoding="UTF-8") as f:
        for line in f:
            content.append(float(line))
    return np.array(content, dtype=np.float32)


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


def check_predictor(predictor: tl2cgen.Predictor, dataset: str) -> None:
    """Check whether a predictor produces correct predictions for a given dataset"""
    dmat = tl2cgen.DMatrix(
        load_svmlight_file(example_model_db[dataset].dtest, zero_based=True)[0],
        dtype=example_model_db[dataset].dtype,
    )
    out_margin = predictor.predict(dmat, pred_margin=True)
    out_prob = predictor.predict(dmat)
    check_predictor_output(dataset, dmat.shape, out_margin, out_prob)


def check_predictor_output(
    dataset: str, shape: Tuple[int, ...], out_margin: np.ndarray, out_prob: np.ndarray
) -> None:
    """Check whether a predictor produces correct predictions"""
    expected_margin = load_txt(example_model_db[dataset].expected_margin)
    if expected_margin is not None:
        if example_model_db[dataset].is_multiclass:
            expected_margin = expected_margin.reshape((shape[0], -1))
        else:
            expected_margin = expected_margin.reshape((-1, 1))
        assert (
            out_margin.shape == expected_margin.shape
        ), f"out_margin.shape = {out_margin.shape}, expected_margin.shape = {expected_margin.shape}"
        np.testing.assert_almost_equal(out_margin, expected_margin, decimal=5)

    expected_prob = load_txt(example_model_db[dataset].expected_prob)
    if expected_prob is not None:
        if example_model_db[dataset].is_multiclass:
            expected_prob = expected_prob.reshape((shape[0], -1))
        else:
            expected_prob = expected_prob.reshape((-1, 1))
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
