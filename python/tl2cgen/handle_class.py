"""Internal classes to hold native handles"""

import ctypes
import pathlib
from typing import Union

import treelite

from .data import DMatrix
from .libloader import _LIB, _check_call
from .util import c_str


class _TreeliteModel:
    """
    Internal class holding a handle to Treelite model. We maintain a separate internal class,
    to maintain **loose coupling** between Treelite and TL2cgen. This way, TL2cgen can support
    past and future versions of Treelite (within the same major version).
    """

    def __init__(self, model: treelite.Model):
        model_bytes = model.serialize_bytes()
        model_bytes_len = len(model_bytes)
        buffer = ctypes.create_string_buffer(model_bytes, model_bytes_len)
        self.handle = ctypes.c_void_p()
        _check_call(
            _LIB.TL2cgenLoadTreeliteModelFromBytes(
                ctypes.pointer(buffer),
                ctypes.c_size_t(model_bytes_len),
                ctypes.byref(self.handle),
            )
        )
        major_ver, minor_ver, patch_ver = (
            ctypes.c_int32(),
            ctypes.c_int32(),
            ctypes.c_int32(),
        )
        _check_call(
            _LIB.TL2cgenQueryTreeliteModelVersion(
                self.handle,
                ctypes.byref(major_ver),
                ctypes.byref(minor_ver),
                ctypes.byref(patch_ver),
            )
        )
        self.__version__ = f"{major_ver.value}.{minor_ver.value}.{patch_ver.value}"

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenFreeTreeliteModel(self.handle))
            self.handle = None


class _Annotator:
    """Annotator object"""

    def __init__(
        self,
        model: _TreeliteModel,
        dmat: DMatrix,
        nthread: int,
        verbose: bool = False,
    ):
        self.handle = ctypes.c_void_p()
        _check_call(
            _LIB.TL2cgenAnnotateBranch(
                model.handle,
                dmat.handle,
                ctypes.c_int(nthread),
                ctypes.c_int(1 if verbose else 0),
                ctypes.byref(self.handle),
            )
        )

    def save(self, path: Union[str, pathlib.Path]):
        """Save annotation data to a JSON file"""
        path = pathlib.Path(path).expanduser().resolve()
        _check_call(_LIB.TL2cgenAnnotationSave(self.handle, c_str(str(path))))

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenAnnotationFree(self.handle))
            self.handle = None
