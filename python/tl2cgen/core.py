"""Core module of TL2cgen"""
import ctypes
import json
import pathlib
from typing import Any, Dict, Optional, Union

import treelite

from .exception import TL2cgenError
from .libloader import _load_lib
from .util import c_str


def _py_version() -> str:
    """Get the TL2cgen version from Python version file."""
    version_file = pathlib.Path(__file__).parent / "VERSION"
    with open(version_file, encoding="utf-8") as f:
        return f.read().strip()


_LIB = _load_lib()


def _check_call(ret):
    """Check the return value of C API call

    This function will raise exception when error occurs.
    Wrap every API call with this function

    Parameters
    ----------
    ret : int
        return value from API calls
    """
    if ret != 0:
        raise TL2cgenError(_LIB.TL2cgenGetLastError().decode("utf-8"))


class _TreeliteModel:
    """Treelite model object"""

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
        """
        Generate prediction code

        Parameters
        ----------
        model :
            Model to convert to C code
        dirpath :
            Directory to store header and source files
        """
        dirpath = pathlib.Path(dirpath).expanduser().resolve()
        _check_call(
            _LIB.TL2cgenCompilerGenerateCode(
                self.handle, model.handle, c_str(str(dirpath))
            )
        )

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenCompilerFree(self.handle))


def generate_c_code(
    model: treelite.Model,
    dirpath: Union[str, pathlib.Path],
    params: Optional[Dict[str, Any]],
    compiler: str = "ast_native",
    verbose: bool = False,
) -> None:
    """
    Generate prediction code from a tree ensemble model. The code will be C99
    compliant. One header file (.h) will be generated, along with one or more
    source files (.c). Use :py:meth:`create_shared` method to package
    prediction code as a dynamic shared library (.so/.dll/.dylib).

    Parameters
    ----------
    model :
        Model to convert to C code
    dirpath :
        Directory to store header and source files
    params :
        Parameters for compiler. See :py:doc:`this page <knobs/compiler_param>`
        for the list of compiler parameters.
    compiler :
        Name of compiler to use
    verbose :
        Whether to print extra messages during compilation

    Example
    -------
    The following populates the directory ``./model`` with source and header
    files:

    .. code-block:: python

       tl2cgen.compile(model, dirpath="./my/model", params={}, verbose=True)

    If parallel compilation is enabled (parameter ``parallel_comp``), the files
    are in the form of ``./my/model/header.h``, ``./my/model/main.c``,
    ``./my/model/tu0.c``, ``./my/model/tu1.c`` and so forth, depending on
    the value of ``parallel_comp``. Otherwise, there will be exactly two files:
    ``./model/header.h``, ``./my/model/main.c``
    """
    _model = _TreeliteModel(model)
    compiler_obj = _Compiler(params, compiler, verbose)
    compiler_obj.compile(_model, dirpath)
