# coding: utf-8
"""
TL2cgen: Model compiler for decision tree ensembles
"""
from .core import _py_version, generate_c_code

__version__ = _py_version()

__all__ = ["generate_c_code"]
