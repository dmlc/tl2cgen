#!/usr/bin/env python
"""Build native lib for tl2cgen4j"""
# pylint: disable=invalid-name
import errno
import os
import shutil
import subprocess
import sys
from contextlib import contextmanager


@contextmanager
def cd(path):
    """Change current working directory temporarily"""
    path = normpath(path)
    cwd = os.getcwd()
    os.chdir(path)
    print("cd " + path)
    try:
        yield path
    finally:
        os.chdir(cwd)


def maybe_makedirs(path):
    """Make directory if not exist"""
    path = normpath(path)
    print("mkdir -p " + path)
    try:
        os.makedirs(path)
    except OSError as e:
        if e.errno != errno.EEXIST:
            raise


def run(command, **kwargs):
    """Run command"""
    print(command)
    subprocess.check_call(command, shell=True, **kwargs)


def cp(source, target):
    """Copy file"""
    source = normpath(source)
    target = normpath(target)
    print(f"cp {source} {target}")
    shutil.copy(source, target)


def normpath(path):
    """Normalize UNIX path to a native path."""
    normalized = os.path.join(*path.split("/"))
    if os.path.isabs(path):
        return os.path.abspath("/") + normalized
    return normalized


if __name__ == "__main__":
    if sys.platform == "darwin":
        os.environ["JAVA_HOME"] = (
            subprocess.check_output("/usr/libexec/java_home").strip().decode()
        )

    print("Building tl2cgen4j library")
    with cd("../.."):
        maybe_makedirs("build")
        with cd("build"):
            if sys.platform == "win32":
                maybe_generator = ' -G"Visual Studio 17 2022" -A x64'
            else:
                maybe_generator = ""
            if sys.platform == "linux":
                maybe_parallel_build = " -- -j$(nproc)"
            else:
                maybe_parallel_build = ""
            if "cpp-coverage" in sys.argv:
                maybe_generator += " -DTEST_COVERAGE=ON"
            run(
                "cmake .. -DBUILD_JVM_RUNTIME=ON -DCMAKE_VERBOSE_MAKEFILE=ON"
                + maybe_generator
            )
            run("cmake --build . --config Release" + maybe_parallel_build)

    print("Copying tl2cgen4j library")
    library_name = {
        "win32": "tl2cgen4j.dll",
        "darwin": "libtl2cgen4j.dylib",
        "linux": "libtl2cgen4j.so",
    }[sys.platform]
    maybe_makedirs("src/main/resources/lib")
    cp("../../build/java_runtime/" + library_name, "src/main/resources/lib")

    print("building mushroom example")
    with cd("src/test/resources/mushroom_example"):
        run("cmake . " + maybe_generator)
        run("cmake --build . --config Release")
