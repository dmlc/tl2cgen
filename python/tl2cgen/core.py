"""Core module of TL2cgen"""
import pathlib
from typing import Any, Dict, Optional, Union

import treelite

from .handle_class import _Compiler, _TreeliteModel


def _py_version() -> str:
    """Get the TL2cgen version from Python version file."""
    version_file = pathlib.Path(__file__).parent / "VERSION"
    with open(version_file, encoding="utf-8") as f:
        return f.read().strip()


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
