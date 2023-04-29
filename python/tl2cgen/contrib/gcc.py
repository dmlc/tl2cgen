"""
Tools to interact with toolchains GCC, Clang, and other UNIX compilers
"""
import pathlib
from typing import Any, Dict, List

from .create_shared import _create_shared_base
from .util import _libext

LIBEXT = _libext()


def _obj_ext() -> str:
    return ".o"


def _obj_cmd(
    source: str,
    toolchain: str,
    options: List[str],
) -> str:
    obj_ext = _obj_ext()
    source_file = source + ".c"
    obj_file = source + obj_ext
    options_str = " ".join(options)
    return (
        f"{toolchain} -c -O3 -o {obj_file} {source_file} -fPIC -std=c99 {options_str}"
    )


def _lib_cmd(
    objects: List[str],
    target: str,
    lib_ext: str,
    toolchain: str,
    options: List[str],
) -> str:
    objects_str = " ".join(objects)
    options_str = " ".join(options)
    return f"{toolchain} -shared -O3 -o {target + lib_ext} {objects_str} -std=c99 {options_str}"


def _create_shared_gcc(
    dirpath: pathlib.Path,
    toolchain: str,
    recipe: Dict[str, Any],
    *,
    nthread: int,
    options: List[str],
    verbose: bool,
) -> pathlib.Path:
    options += ["-lm"]
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
    recipe["initial_cmd"] = ""
    return _create_shared_base(dirpath, recipe, nthread=nthread, verbose=verbose)
