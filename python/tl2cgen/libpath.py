# coding: utf-8
"""Find the path to TL2cgen dynamic library files."""

import os
import pathlib
import platform
import sys
from typing import List


class TL2cgenLibraryNotFound(Exception):
    """Error thrown by when TL2cgen is not found"""


def find_lib_path() -> List[pathlib.Path]:
    """Find the path to TL2cgen dynamic library files.

    Returns
    -------
    lib_path
       List of all found library path to TL2cgen
    """
    curr_path = pathlib.Path(__file__).expanduser().absolute().parent
    dll_path = [
        # When installed, libtreelite will be installed in <site-package-dir>/lib
        curr_path / "lib",
        # Editable installation
        curr_path.parent / "build",
        # Use libtreelite from a system prefix, if available. This should be the last option.
        pathlib.Path(sys.prefix).absolute().resolve() / "lib",
    ]

    if sys.platform == "win32":
        if platform.architecture()[0] == "64bit":
            dll_path.append(curr_path.joinpath("../../windows/x64/Release/"))
            dll_path.append(curr_path.joinpath("./windows/x64/Release/"))
        else:
            dll_path.append(curr_path.joinpath("../../windows/Release/"))
            dll_path.append(curr_path.joinpath("./windows/Release/"))
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

    # XGBOOST_BUILD_DOC is defined by sphinx conf.
    if not lib_path and not os.environ.get("XGBOOST_BUILD_DOC", False):
        link = "https://tl2cgen.readthedocs.io/en/latest/install.html"
        msg = (
            "Cannot find TL2cgen Library in the candidate path.  "
            + "List of candidates:\n- "
            + ("\n- ".join(str(x) for x in dll_path))
            + "\nXGBoost Python package path: "
            + str(curr_path)
            + "\nsys.prefix: "
            + sys.prefix
            + "\nSee: "
            + link
            + " for installing TL2cgen."
        )
        raise TL2cgenLibraryNotFound(msg)
    return lib_path
