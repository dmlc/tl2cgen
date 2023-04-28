"""Utility functions"""
import ctypes


def c_str(string):
    """Convert a Python string to C string"""
    return ctypes.c_char_p(string.encode("utf-8"))


def py_str(string):
    """Convert C string back to Python string"""
    return string.decode("utf-8")
