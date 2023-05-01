"""
Tools to interact with Microsoft Visual C++ (MSVC)
"""
import itertools
import os
import pathlib
import re
import sys
from typing import Any, Dict, List

from packaging.version import parse as parse_version

from .create_shared import _create_shared_base
from .util import _libext

LIBEXT = _libext()


if sys.platform != "win32":

    class WindowsError(Exception):  # pylint: disable=C0115,W0622
        pass


def _is_64bit_windows() -> bool:
    return "PROGRAMFILES(X86)" in os.environ


def _varsall_bat_path() -> pathlib.Path:  # pylint: disable=R0912
    if sys.platform != "win32":
        raise RuntimeError("_varsall_bat_path() supported only on Windows")

    # if a custom location is given, try that first
    if "TL2CGEN_VCVARSALL" in os.environ:
        candidate = pathlib.Path(os.environ["TL2CGEN_VCVARSALL"]).resolve()
        if candidate.name.lower() != "vcvarsall.bat":
            raise OSError(
                "Environment variable TL2CGEN_VCVARSALL must point to file vcvarsall.bat"
            )
        if candidate.is_file():
            return candidate
        raise OSError(
            "Environment variable TL2CGEN_VCVARSALL does not refer to existing vcvarsall.bat"
        )

    # == Bunch of heuristics to locate vcvarsall.bat ==

    # List of possible paths to vcvarsall.bat
    candidate_paths = []
    try:
        import winreg  # pylint: disable=E0401,C0415

        if _is_64bit_windows():
            key_name = r"SOFTWARE\Wow6432Node\Microsoft\VisualStudio\SxS\VS7"
        else:
            key_name = r"SOFTWARE\Microsoft\VisualStudio\SxS\VC7"
        key = winreg.OpenKey(winreg.HKEY_LOCAL_MACHINE, key_name)
        i = 0
        while True:
            try:
                version, vcroot, _ = winreg.EnumValue(key, i)
                vcroot = pathlib.Path(vcroot).resolve()
                if parse_version(version) >= parse_version("15.0"):
                    # Visual Studio 2017 revamped directory structure
                    candidate_paths.append(vcroot / r"VC\Auxiliary\Build\vcvarsall.bat")
                else:
                    candidate_paths.append(vcroot / r"VC\vcvarsall.bat")
            except WindowsError:  # pylint: disable=E0602
                break
            i += 1
    except FileNotFoundError:
        pass  # No registry key found
    except ImportError:
        pass  # No winreg module

    for candidate in candidate_paths:
        if candidate.is_file():
            return candidate

    # If registry method fails, try a bunch of pre-defined paths

    # Visual Studio 2017 and higher
    candidate_paths = list(
        itertools.chain(
            pathlib.Path(r"C:\Program Files (x86)\Microsoft Visual Studio").glob("*"),
            pathlib.Path(r"C:\Program Files\Microsoft Visual Studio").glob("*"),
        )
    )
    for vcroot in candidate_paths:
        if re.fullmatch(r"[0-9]+", vcroot.name):
            for candidate in vcroot.glob(r"*\VC\Auxiliary\Build\vcvarsall.bat"):
                if candidate.is_file():
                    return candidate
    # Previous versions of Visual Studio
    candidate_paths = list(
        itertools.chain(
            pathlib.Path(r"C:\Program Files (x86)").glob(
                r"Microsoft Visual Studio*\VC\vcvarsall.bat"
            ),
            pathlib.Path(r"C:\Program Files").glob(
                r"Microsoft Visual Studio*\VC\vcvarsall.bat"
            ),
        )
    )
    for candidate in candidate_paths:
        if candidate.is_file():
            return candidate

    raise OSError(
        "vcvarsall.bat not found; please specify its full path in the environment "
        "variable TL2CGEN_VCVARSALL"
    )


def _obj_ext() -> str:
    return ".obj"


# pylint: disable=W0613
def _obj_cmd(
    source: str,
    toolchain: str,
    options: List[str],
):
    source_file = source + ".c"
    options_str = " ".join(options)
    return f"cl.exe /c /openmp /Ox {source_file} {options_str}"


# pylint: disable=W0613
def _lib_cmd(
    objects: List[str],
    target: str,
    lib_ext: str,
    toolchain: str,
    options: List[str],
) -> str:
    objects_str = " ".join(objects)
    options_str = " ".join(options)
    return f"cl.exe /LD /Fe{target} /openmp {objects_str} {options_str}"


# pylint: disable=R0913
def _create_shared_msvc(
    dirpath: pathlib.Path,
    toolchain: str,
    recipe: Dict[str, Any],
    *,
    nthread: int,
    options: List[str],
    verbose: bool,
) -> pathlib.Path:
    # Specify command to compile an object file
    recipe["object_ext"] = _obj_ext()
    recipe["library_ext"] = LIBEXT

    # pylint: disable=R0801

    def _obj_cmd_wrapped(source: str) -> str:
        return _obj_cmd(source, toolchain, options)

    def _lib_cmd_wrapped(objects: List[str], target: str) -> str:
        return _lib_cmd(objects, target, LIBEXT, toolchain, options)

    recipe["create_object_cmd"] = _obj_cmd_wrapped
    recipe["create_library_cmd"] = _lib_cmd_wrapped
    plat_target = "amd64" if _is_64bit_windows() else "x86"
    recipe["initial_cmd"] = f'"{_varsall_bat_path()}" {plat_target}'
    return _create_shared_base(dirpath, recipe, nthread=nthread, verbose=verbose)
