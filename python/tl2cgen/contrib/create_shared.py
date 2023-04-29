"""Logic for launching C compiler to build shared libs"""
import pathlib
import subprocess
from typing import Any, Dict

from ..exception import TL2cgenError
from .util import (
    _create_log_cmd_unix,
    _create_log_cmd_windows,
    _is_windows,
    _save_retcode_cmd_unix,
    _save_retcode_cmd_windows,
    _shell,
)


def _enqueue(args: Dict[str, Any]) -> subprocess.Popen:
    tid = args["tid"]
    queue = args["queue"]
    dirpath = args["dirpath"]
    init_cmd = args["init_cmd"]
    create_log_cmd = args["create_log_cmd"]
    save_retcode_cmd = args["save_retcode_cmd"]

    # pylint: disable=R1732
    proc = subprocess.Popen(
        _shell(),
        shell=True,
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        cwd=dirpath,
    )
    assert proc.stdin is not None
    proc.stdin.write((init_cmd + "\n").encode())
    proc.stdin.write(create_log_cmd(f"retcode_cpu{tid}.txt" + "\n").encode())
    for command in queue:
        proc.stdin.write((command + "\n").encode())
        proc.stdin.write((save_retcode_cmd(f"retcode_cpu{tid}.txt") + "\n").encode())
    proc.stdin.flush()

    return proc


def _wait(proc: subprocess.Popen, args: Dict[str, Any]):
    tid = args["tid"]
    dirpath = args["dirpath"]
    stdout, _ = proc.communicate()
    with open(dirpath / f"retcode_cpu{tid}.txt", "r", encoding="UTF-8") as f:
        retcode = [int(line) for line in f]
    return {"stdout": stdout.decode(), "retcode": retcode}


def _create_shared_base(
    dirpath: pathlib.Path,
    recipe: Dict[str, Any],
    *,
    nthread: int,
    verbose: bool,
):  # pylint: disable=R0914
    # Fetch toolchain-specific commands
    obj_cmd = recipe["create_object_cmd"]
    lib_cmd = recipe["create_library_cmd"]
    create_log_cmd = _create_log_cmd_windows if _is_windows() else _create_log_cmd_unix
    save_retcode_cmd = (
        _save_retcode_cmd_windows if _is_windows() else _save_retcode_cmd_unix
    )

    # 1. Compile sources in parallel
    if verbose:
        obj_ext = recipe["object_ext"]
        print(
            f"Compiling sources files in directory {dirpath} into object files (*{obj_ext})..."
        )
    workqueue = [
        {
            "tid": tid,
            "queue": [],
            "dirpath": dirpath,
            "init_cmd": recipe["initial_cmd"],
            "create_log_cmd": create_log_cmd,
            "save_retcode_cmd": save_retcode_cmd,
        }
        for tid in range(nthread)
    ]
    for i, source in enumerate(recipe["sources"]):
        workqueue[i % nthread]["queue"].append(obj_cmd(source["name"]))
    proc = [_enqueue(workqueue[tid]) for tid in range(nthread)]
    result = []
    for tid in range(nthread):
        result.append(_wait(proc[tid], workqueue[tid]))

    for tid in range(nthread):
        if not all(x == 0 for x in result[tid]["retcode"]):
            log_path = dirpath / f"log_cpu{tid}.txt"
            with open(log_path, "w", encoding="UTF-8") as f:
                f.write(result[tid]["stdout"] + "\n")
            raise TL2cgenError(
                f"Error occured in worker #{tid}: " + result[tid]["stdout"]
            )

    # 2. Package objects into a dynamic shared library
    full_libpath = dirpath.joinpath(recipe["target"] + recipe["library_ext"])
    if verbose:
        print(f"Generating dynamic shared library {full_libpath}...")
    objects = [x["name"] + recipe["object_ext"] for x in recipe["sources"]]
    objects.extend(recipe.get("extra", []))
    workqueue = [
        {
            "tid": 0,
            "queue": [lib_cmd(objects, recipe["target"])],
            "dirpath": dirpath,
            "init_cmd": recipe["initial_cmd"],
            "create_log_cmd": create_log_cmd,
            "save_retcode_cmd": save_retcode_cmd,
        }
    ]
    proc = [_enqueue(workqueue[0])]
    result = [_wait(proc[0], workqueue[0])]

    if result[0]["retcode"][0] != 0:
        with open(dirpath / "log_cpu0.txt", "w", encoding="UTF-8") as f:
            f.write(result[0]["stdout"] + "\n")
        raise TL2cgenError(
            "Error occured while creating dynamic library: " + result[0]["stdout"]
        )

    # 3. Clean up
    for tid in range(nthread):
        dirpath.joinpath(f"retcode_cpu{tid}.txt").unlink()

    # Return full path of shared library
    return dirpath.joinpath(full_libpath)
