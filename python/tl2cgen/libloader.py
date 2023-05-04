# coding: utf-8
"""Find the path to TL2cgen dynamic library files."""

import ctypes
import os
import pathlib
import sys
import warnings
from typing import List

from .exception import TL2cgenError, TL2cgenLibraryNotFound
from .util import py_str


def _find_lib_path() -> List[pathlib.Path]:
    """Find the path to TL2cgen dynamic library files.

    Returns
    -------
    lib_path
       List of all found library path to TL2cgen
    """
    curr_path = pathlib.Path(__file__).expanduser().absolute().parent
    dll_path = [
        # When installed, libtl2cgen will be installed in <site-package-dir>/lib
        curr_path / "lib",
        # Editable installation
        curr_path.parent.parent / "build",
        # Use libtl2cgen from a system prefix, if available. This should be the last option.
        pathlib.Path(sys.prefix).expanduser().resolve() / "lib",
    ]

    if sys.platform == "win32":
        # On Windows, Conda may install libs in different paths
        sys_prefix = pathlib.Path(sys.prefix)
        dll_path.extend(
            [
                sys_prefix / "bin",
                sys_prefix / "Library",
                sys_prefix / "Library" / "bin",
                sys_prefix / "Library" / "lib",
            ]
        )
        dll_path = [p.joinpath("tl2cgen.dll") for p in dll_path]
    elif sys.platform.startswith(("linux", "freebsd", "emscripten", "OS400")):
        dll_path = [p.joinpath("libtl2cgen.so") for p in dll_path]
    elif sys.platform == "darwin":
        dll_path = [p.joinpath("libtl2cgen.dylib") for p in dll_path]
    elif sys.platform == "cygwin":
        dll_path = [p.joinpath("cygtl2cgen.dll") for p in dll_path]
    else:
        raise RuntimeError(f"Unrecognized platform: {sys.platform}")

    lib_path = [p for p in dll_path if p.exists() and p.is_file()]

    # TL2CGEN_BUILD_DOC is defined by sphinx conf.
    if not lib_path and not os.environ.get("TL2CGEN_BUILD_DOC", False):
        link = "https://tl2cgen.readthedocs.io/en/latest/install.html"
        msg = (
            "Cannot find TL2cgen Library in the candidate path.  "
            + "List of candidates:\n- "
            + ("\n- ".join(str(x) for x in dll_path))
            + "\nTL2cgen Python package path: "
            + str(curr_path)
            + "\nsys.prefix: "
            + sys.prefix
            + "\nSee: "
            + link
            + " for installing TL2cgen."
        )
        raise TL2cgenLibraryNotFound(msg)
    return lib_path


@ctypes.CFUNCTYPE(None, ctypes.c_char_p)
def _log_callback(msg: bytes) -> None:
    """Redirect logs from native library into Python console"""
    print(msg.decode("utf-8"))


@ctypes.CFUNCTYPE(None, ctypes.c_char_p)
def _warn_callback(msg: bytes) -> None:
    """Redirect warnings from native library into Python console"""
    warnings.warn(msg.decode("utf-8"))


def _load_lib() -> ctypes.CDLL:
    """Load TL2cgen Library."""
    lib_paths = _find_lib_path()
    if not lib_paths:
        # This happens only when building document.
        return None  # type: ignore
    if sys.version_info >= (3, 8) and sys.platform == "win32":
        # pylint: disable=no-member
        os.add_dll_directory(
            os.path.join(os.path.normpath(sys.prefix), "Library", "bin")
        )
    os_error_list = []
    lib = None
    for lib_path in lib_paths:
        try:
            lib = ctypes.cdll.LoadLibrary(str(lib_path))
            setattr(lib, "path", lib_path)
        except OSError as e:
            os_error_list.append(str(e))
            continue
    if not lib:
        libname = lib_paths[0].name
        raise TL2cgenError(
            f"""
TL2cgen Library ({libname}) could not be loaded.
Likely causes:
  * OpenMP runtime is not installed
    - vcomp140.dll or libgomp-1.dll for Windows
    - libomp.dylib for Mac OSX
    - libgomp.so for Linux and other UNIX-like OSes
    Mac OSX users: Run `brew install libomp` to install OpenMP runtime.
  * You are running 32-bit Python on a 64-bit OS
Error message(s): {os_error_list}
"""
        )
    lib.TL2cgenGetLastError.restype = ctypes.c_char_p
    lib.log_callback = _log_callback  # type: ignore
    lib.warn_callback = _warn_callback  # type: ignore
    if lib.TL2cgenRegisterLogCallback(lib.log_callback) != 0:
        raise TL2cgenError(py_str(lib.TL2cgenGetLastError()))
    if lib.TL2cgenRegisterWarningCallback(lib.warn_callback) != 0:
        raise TL2cgenError(py_str(lib.TL2cgenGetLastError()))
    return lib


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


# Load native library in the global scope
_LIB = _load_lib()
