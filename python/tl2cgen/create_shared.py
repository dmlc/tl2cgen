"""Launcher for C compiler to build shared libs"""
import pathlib
import time
import warnings
from multiprocessing import cpu_count
from typing import List, Optional, Union

from .contrib.gcc import _create_shared_gcc
from .contrib.msvc import _create_shared_msvc
from .contrib.util import _toolchain_exist_check
from .exception import TL2cgenError
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
        Which toolchain to use. You may choose one of "msvc", "clang", and "gcc".
        You may also specify a specific variation of clang or gcc (e.g. "gcc-7")
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
