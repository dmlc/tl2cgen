"""Internal classes to hold native handles"""
import ctypes
import json
import pathlib
from typing import Any, Dict, Optional, Union

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


class _Compiler:
    """Compiler object"""

    def __init__(
        self,
        params: Optional[Dict[str, Any]],
        compiler: str = "ast_native",
        verbose: bool = False,
    ):
        self.handle = ctypes.c_void_p()
        if params is None:
            params = {}
        if verbose:
            params["verbose"] = 1
        params_json_str = json.dumps(params)
        _check_call(
            _LIB.TL2cgenCompilerCreate(
                c_str(compiler), c_str(params_json_str), ctypes.byref(self.handle)
            )
        )

    def compile(self, model: _TreeliteModel, dirpath: Union[str, pathlib.Path]) -> None:
        """Generate prediction code"""
        dirpath = pathlib.Path(dirpath).expanduser().resolve()
        _check_call(
            _LIB.TL2cgenCompilerGenerateCode(
                self.handle, model.handle, c_str(str(dirpath))
            )
        )

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenCompilerFree(self.handle))
            self.handle = None
