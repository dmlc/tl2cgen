"""Wrapper for Pylint"""

import os
import pathlib
import subprocess

ROOT_PATH = pathlib.Path(__file__).parent.parent.expanduser().resolve()
PYLINTRC_PATH = ROOT_PATH / "python" / ".pylintrc"
PYPKG_PATH = ROOT_PATH / "python" / "tl2cgen"

SCANNED_DIRS = [
    PYPKG_PATH,
    ROOT_PATH / "python" / "packager",
    ROOT_PATH / "dev",
    ROOT_PATH / "tests" / "python",
]


def main():
    """Wrapper for Pylint. Add tl2cgen to PYTHONPATH so that pylint doesn't error out"""
    new_env = os.environ.copy()
    new_env["PYTHONPATH"] = str(PYPKG_PATH)

    input_args = [str(dir) for dir in SCANNED_DIRS]
    subprocess.run(
        ["pylint", "-rn", "-sn", "--rcfile", str(PYLINTRC_PATH)] + input_args,
        check=True,
        env=new_env,
    )


if __name__ == "__main__":
    main()
