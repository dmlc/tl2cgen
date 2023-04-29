"""Generator for Makefile and CMakeLists.txt"""
import pathlib
from typing import List, Optional, Union

from .contrib import gcc, msvc
from .contrib.util import _libext, _toolchain_exist_check
from .exception import TL2cgenError
from .util import _open_and_validate_recipe, _process_options


def generate_makefile(
    dirpath: Union[str, pathlib.Path],
    toolchain: str,
    options: Optional[List[str]] = None,
) -> None:
    """
    Generate a Makefile for a given directory of headers and sources. The
    resulting Makefile will be stored in the directory. This function is useful
    for deploying a model on a different machine.

    Parameters
    ----------
    dirpath :
        Directory containing the header and source files previously generated
        by :py:meth:`Model.compile`. The directory must contain recipe.json
        which specifies build dependencies.
    toolchain :
        Which toolchain to use. You may choose one of 'msvc', 'clang', and 'gcc'.
        You may also specify a specific variation of clang or gcc (e.g. 'gcc-7')
    options :
        Additional options to pass to toolchain
    """
    # pylint: disable=R0912,R0914,W0212
    dirpath = pathlib.Path(dirpath).expanduser().resolve()
    if not dirpath.is_dir():
        raise TL2cgenError(f"Directory {dirpath} does not exist")
    recipe = _open_and_validate_recipe(dirpath / "recipe.json")
    options = _process_options(options)

    # Determine file extensions for object and library files
    _toolchain_exist_check(toolchain)
    if toolchain == "msvc":
        _obj_ext = msvc._obj_ext
        _obj_cmd = msvc._obj_cmd
        _lib_cmd = msvc._lib_cmd
    else:
        _obj_ext = gcc._obj_ext
        _obj_cmd = gcc._obj_cmd
        _lib_cmd = gcc._lib_cmd
    obj_ext = _obj_ext()
    lib_ext = _libext()

    with open(dirpath / "Makefile", "w", encoding="UTF-8") as f:
        objects = [x["name"] + obj_ext for x in recipe["sources"]]
        objects.extend(recipe.get("extra", []))
        target = recipe["target"] + lib_ext
        objects_str = " ".join(objects)
        lib_cmd = _lib_cmd(
            objects=objects,
            target=recipe["target"],
            lib_ext=lib_ext,
            toolchain=toolchain,
            options=options,
        )

        print(f"{target}: {objects_str}", file=f)
        print(f"\t{lib_cmd}", file=f)
        for source in recipe["sources"]:
            source_file = source["name"] + ".c"
            obj_file = source["name"] + obj_ext
            obj_cmd = _obj_cmd(
                source=source["name"], toolchain=toolchain, options=options
            )
            print(f"{obj_file}: {source_file}", file=f)
            print(f"\t{obj_cmd}", file=f)


def generate_cmakelists(
    dirpath: Union[str, pathlib.Path],
    options: Optional[List[str]] = None,
) -> None:
    """
    Generate a CMakeLists.txt for a given directory of headers and sources. The
    resulting CMakeLists.txt will be stored in the directory. This function is useful
    for deploying a model on a different machine.

    Parameters
    ----------
    dirpath :
        Directory containing the header and source files previously generated
        by :py:meth:`Model.compile`. The directory must contain recipe.json
        which specifies build dependencies.
    options :
        Additional options to pass to toolchain
    """
    dirpath = pathlib.Path(dirpath).expanduser().resolve()
    if not dirpath.is_dir():
        raise TL2cgenError(f"Directory {dirpath} does not exist")
    recipe = _open_and_validate_recipe(dirpath / "recipe.json")
    options = _process_options(options)

    target = recipe["target"]
    sources = " ".join([x["name"] + ".c" for x in recipe["sources"]])
    options_str = " ".join(options)
    with open(dirpath / "CMakeLists.txt", "w", encoding="UTF-8") as f:
        print("cmake_minimum_required(VERSION 3.13)", file=f)
        print("project(mushroom LANGUAGES C)\n", file=f)
        print(f"add_library({target} SHARED)", file=f)
        print(f"target_sources({target} PRIVATE header.h {sources})", file=f)
        print(f"target_compile_options({target} PRIVATE {options_str})", file=f)
        print(
            f'target_include_directories({target} PRIVATE "${{PROJECT_BINARY_DIR}}")',
            file=f,
        )
        print(f"set_target_properties({target} PROPERTIES", file=f)
        print(
            """POSITION_INDEPENDENT_CODE ON
            C_STANDARD 99
            C_STANDARD_REQUIRED ON
            PREFIX ""
            RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}"
            RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELEASE "${PROJECT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}"
            LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_BINARY_DIR}")
        """,
            file=f,
        )
