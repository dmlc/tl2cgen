# coding: utf-8
"""
TL2cgen: Model compiler for decision tree ensembles
"""
from .core import _py_version, annotate_branch, generate_c_code
from .data import DMatrix

__version__ = _py_version()

__all__ = ["annotate_branch", "generate_c_code", "DMatrix"]
