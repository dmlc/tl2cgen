"""Launcher for C compiler to build shared libs"""
import pathlib
import shutil
import time
import warnings
from multiprocessing import cpu_count
from tempfile import TemporaryDirectory
from typing import Any, Dict, List, Optional, Union

import treelite

from .contrib.gcc import _create_shared_gcc
from .contrib.msvc import _create_shared_msvc
from .contrib.util import _toolchain_exist_check
from .core import generate_c_code
from .exception import TL2cgenError
from .generate_makefile import generate_cmakelists, generate_makefile
from .util import _open_and_validate_recipe, _process_options


def create_shared(
    toolchain: str,
    dirpath: Union[str, pathlib.Path],
    *,
    nthread: Optional[int] = None,
    verbose: bool = False,
    options: Optional[List[str]] = None,
    long_build_time_warning: bool = True,
):  # pylint: disable=R0914
    """Create shared library.

    Parameters
    ----------
    toolchain :
        Which toolchain to use. You may choose one of 'msvc', 'clang', and 'gcc'.
        You may also specify a specific variation of clang or gcc (e.g. 'gcc-7')
    dirpath :
        Directory containing the header and source files previously generated
        by :py:meth:`generate_c_code`. The directory must contain recipe.json
        which specifies build dependencies.
    nthread :
        Number of threads to use in creating the shared library.
        Defaults to the number of cores in the system.
    verbose :
        Whether to produce extra messages
    options :
        Additional options to pass to toolchain
    long_build_time_warning :
        If set to False, suppress the warning about potentially long build time

    Returns
    -------
    libpath :
        Absolute path of created shared library

    Example
    -------
    The following command uses Visual C++ toolchain to generate
    ``./my/model/model.dll``:

    .. code-block:: python

        tl2cgen.generate_c_code(model, dirpath="./my/model",
                                params={})
        tl2cgen.create_shared(toolchain="msvc", dirpath="./my/model")

    Later, the shared library can be referred to by its directory name:

    .. code-block:: python

        predictor = tl2cgen.Predictor(libpath="./my/model")
        # looks for ./my/model/model.dll

    Alternatively, one may specify the library down to its file name:

    .. code-block:: python

        predictor = tl2cgen.Predictor(libpath="./my/model/model.dll")
    """

    # pylint: disable=R0912

    if nthread is None or nthread <= 0:
        ncore = cpu_count()
        nthread = ncore
    dirpath = pathlib.Path(dirpath).expanduser().resolve()
    if not dirpath.exists() or not dirpath.is_dir():
        raise TL2cgenError(f"Directory {dirpath} does not exist")
    recipe = _open_and_validate_recipe(dirpath / "recipe.json")
    options = _process_options(options)

    # Write warning for potentially long compile time
    if long_build_time_warning:
        warn = False
        for source in recipe["sources"]:
            if int(source["length"]) > 10000:
                warn = True
                break
        if warn:
            warnings.warn(
                "WARNING: some of the source files are long. Expect long build time. "
                "You may want to adjust the parameter 'parallel_comp'."
            )

    tstart = time.perf_counter()
    _toolchain_exist_check(toolchain)
    if toolchain == "msvc":
        _create_shared = _create_shared_msvc
    else:
        _create_shared = _create_shared_gcc
    libpath = _create_shared(
        dirpath, toolchain, recipe, nthread=nthread, options=options, verbose=verbose
    )
    if verbose:
        elapsed_time = time.perf_counter() - tstart
        print(f"Generated shared library in {elapsed_time:.2f} seconds")
    return libpath


def export_lib(
    model: treelite.Model,
    toolchain: str,
    libpath: Union[str, pathlib.Path],
    params: Optional[Dict[str, Any]] = None,
    compiler: str = "ast_native",
    *,
    nthread: Optional[int] = None,
    verbose: bool = False,
    options: Optional[List[str]] = None,
):
    """
    Convenience function: Generate prediction code and immediately turn it
    into a dynamic shared library. A temporary directory will be created to
    hold the source files.

    Parameters
    ----------
    model :
        Model to convert to C code
    toolchain :
        Which toolchain to use. You may choose one of 'msvc', 'clang', and 'gcc'.
        You may also specify a specific variation of clang or gcc (e.g. 'gcc-7')
    libpath :
        Location to save the generated dynamic shared library
    params :
        Parameters to be passed to the compiler. See
        :py:doc:`this page <knobs/compiler_param>` for the list of compiler
        parameters.
    compiler :
        Kind of C code generator to use. Currently, there are two possible values:
        {"ast_native", "failsafe"}
    nthread :
        Number of threads to use in creating the shared library.
        Defaults to the number of cores in the system.
    verbose :
        Whether to produce extra messages
    options :
        Additional options to pass to toolchain

    Example
    -------
    The one-line command

    .. code-block:: python

        tl2cgen.export_lib(model, toolchain="msvc", libpath="./mymodel.dll",
                           params={})

    is equivalent to the following sequence of commands:

    .. code-block:: python
        tl2cgen.generate_c_code(model, dirpath="/temporary/directory",
                                params={})
        tl2cgen.create_shared(toolchain="msvc",
                              dirpath="/temporary/directory")
        # Move the library out of the temporary directory
        shutil.move("/temporary/directory/mymodel.dll", "./mymodel.dll")
    """
    _toolchain_exist_check(toolchain)
    libpath = pathlib.Path(libpath).expanduser().resolve()
    long_build_time_warning = not (params and "parallel_comp" in params)

    with TemporaryDirectory(dir=libpath) as tempdir:
        generate_c_code(model, tempdir, params, compiler, verbose=verbose)
        temp_libpath = create_shared(
            toolchain,
            tempdir,
            nthread=nthread,
            verbose=verbose,
            options=options,
            long_build_time_warning=long_build_time_warning,
        )
        if libpath.is_file():
            libpath.unlink()
        shutil.move(temp_libpath, libpath)


def export_srcpkg(
    model: treelite.Model,
    toolchain: str,
    pkgpath: Union[str, pathlib.Path],
    libname: str,
    params: Optional[Dict[str, Any]] = None,
    compiler: str = "ast_native",
    *,
    verbose: bool = False,
    options: Optional[List[str]] = None,
):  # pylint: disable=R0913
    """
    Convenience function: Generate prediction code and create a zipped source
    package for deployment. The resulting zip file will also contain a Makefile.

    Parameters
    ----------
    model :
        Model to convert to C code
    toolchain :
        Which toolchain to use. You may choose one of 'msvc', 'clang', 'gcc', and 'cmake'.
        You may also specify a specific variation of clang or gcc (e.g. 'gcc-7')
    pkgpath :
        Location to save the zipped source package
    libname :
        Name of model shared library to be built
    params :
        Parameters to be passed to the compiler. See
        :py:doc:`this page <knobs/compiler_param>` for the list of compiler
        parameters.
    compiler :
        Name of compiler to use in C code generation
    verbose :
        Whether to produce extra messages
    options :
        Additional options to pass to toolchain

    Example
    -------
    The one-line command

    .. code-block:: python

        tl2cgen.export_srcpkg(model, toolchain="gcc",
                              pkgpath="./mymodel_pkg.zip",
                              libname="mymodel.so", params={})

    is equivalent to the following sequence of commands:

    .. code-block:: python

        tl2cgen.generate_c_code(model, dirpath="/temporary/directory/mymodel",
                                params={})
        tl2cgen.generate_makefile(dirpath="/temporary/directory/mymodel",
                                  toolchain="gcc")
        # Zip the directory containing C code and Makefile
        shutil.make_archive(base_name="./mymodel_pkg", format="zip",
                            root_dir="/temporary/directory",
                            base_dir="mymodel/")
    """
    # Check for file extension
    pkgpath = pathlib.Path(pkgpath).expanduser().resolve()
    if pkgpath.suffix != ".zip":
        raise ValueError("Source package file should have .zip extension")
    if toolchain != "cmake":
        _toolchain_exist_check(toolchain)

    with TemporaryDirectory() as temp_dir:
        target = pathlib.Path(libname).stem
        # Create a child directory to get desired name for target
        dirpath = pathlib.Path(temp_dir) / target
        dirpath.mkdir()
        if params is None:
            params = {}
        params["native_lib_name"] = target
        generate_c_code(model, dirpath, params, compiler, verbose=verbose)
        if toolchain == "cmake":
            generate_cmakelists(dirpath, options)
        else:
            generate_makefile(dirpath, toolchain, options)
        shutil.make_archive(
            base_name=str(pkgpath.with_suffix("")),
            format="zip",
            root_dir=temp_dir,
            base_dir=target,
        )
