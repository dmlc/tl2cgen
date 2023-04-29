"""Utilities for contrib module"""
import os
import subprocess
from sys import platform as _platform


def _is_windows() -> bool:
    return _platform == "win32"


def _shell() -> str:
    if _is_windows():
        return "cmd.exe"
    if "SHELL" in os.environ:
        return os.environ["SHELL"]
    return "/bin/sh"  # use POSIX-compliant shell if SHELL is not set


def _libext():
    if _platform == "darwin":
        return ".dylib"
    if _platform in ("win32", "cygwin"):
        return ".dll"
    return ".so"


def _toolchain_exist_check(toolchain: str) -> None:
    if toolchain != "msvc":
        retcode = subprocess.call(
            f"{toolchain} --version",
            shell=True,
            stdin=subprocess.DEVNULL,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        )
        if retcode != 0:
            raise ValueError(
                f"Toolchain {toolchain} not found. Ensure that it is installed and "
                "that it is a variant of GCC or Clang."
            )


def _create_log_cmd_unix(logfile: str) -> str:
    return f"true > {logfile}"


def _save_retcode_cmd_unix(logfile: str) -> str:
    if _shell().endswith("fish"):  # special handling for fish shell
        return f"echo $status >> {logfile}"
    return f"echo $? >> {logfile}"


def _create_log_cmd_windows(logfile: str) -> str:
    return f"type NUL > {logfile}"


def _save_retcode_cmd_windows(logfile: str) -> str:
    return f"echo %errorlevel% >> {logfile}"
