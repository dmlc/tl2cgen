"""Utility functions"""
import ctypes
import json
import pathlib
from typing import Any, Dict, List, Optional

from .exception import TL2cgenError


def c_str(string):
    """Convert a Python string to C string"""
    return ctypes.c_char_p(string.encode("utf-8"))


def py_str(string):
    """Convert C string back to Python string"""
    return string.decode("utf-8")


def _open_and_validate_recipe(recipe_path: pathlib.Path) -> Dict[str, Any]:
    """Ensure that the build recipe contains necessary fields"""
    try:
        with open(recipe_path, "r", encoding="utf-8") as f:
            recipe = json.load(f)
    except IOError as e:
        raise TL2cgenError("Failed to open recipe.json") from e
    if "sources" not in recipe or "target" not in recipe:
        raise TL2cgenError("Malformed recipe.json")
    return recipe


def _process_options(options: Optional[List[str]]) -> List[str]:
    """Ensure that options are in the form of list of strings."""
    if options is not None:
        try:
            _ = iter(options)
            options = [str(x) for x in options]
        except TypeError as e:
            raise TL2cgenError("options must be a list of string") from e
    else:
        options = []
    return options
