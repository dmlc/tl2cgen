"""Core module of TL2cgen"""
import pathlib

from .libloader import _load_lib


def _py_version() -> str:
    """Get the XGBoost version from Python version file."""
    version_file = pathlib.Path(__file__).parent / "VERSION"
    with open(version_file, encoding="utf-8") as f:
        return f.read().strip()


_LIB = _load_lib()
