"""Utility functions to handle types"""

import ctypes
from typing import Any, Dict

import numpy as np
import numpy.typing as npt

_CTYPES_TYPE_TABLE: Dict[str, Any] = {
    "uint32": ctypes.c_uint32,
    "float32": ctypes.c_float,
    "float64": ctypes.c_double,
}

_NUMPY_TYPE_TABLE: Dict[str, npt.DTypeLike] = {
    "uint32": np.uint32,
    "float32": np.float32,
    "float64": np.float64,
}


def type_info_to_ctypes_type(type_info: str) -> Any:
    """Obtain ctypes type corresponding to a given TypeInfo"""
    return _CTYPES_TYPE_TABLE[type_info]


def type_info_to_numpy_type(type_info: str) -> npt.DTypeLike:
    """Obtain ctypes type corresponding to a given TypeInfo"""
    return _NUMPY_TYPE_TABLE[type_info]


def numpy_type_to_type_info(type_info: npt.DTypeLike) -> str:
    """Obtain TypeInfo corresponding to a given NumPy type"""
    if type_info == np.uint32:
        return "uint32"
    if type_info == np.float32:
        return "float32"
    if type_info == np.float64:
        return "float64"
    raise ValueError(f"Unrecognized NumPy type: {type_info}")
