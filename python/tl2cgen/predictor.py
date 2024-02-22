"""
Predictor module
"""

import ctypes
import pathlib
from typing import Optional, Union

import numpy as np

from .contrib.util import _libext
from .data import DMatrix
from .exception import TL2cgenError
from .libloader import _LIB, _check_call
from .util import c_str, py_str


class Predictor:
    """
    Predictor class is a convenient wrapper for loading shared libs.
    TL2cgen uses OpenMP to launch multiple CPU threads to perform predictions
    in parallel.

    Parameters
    ----------
    libpath :
        location of dynamic shared library (.dll/.so/.dylib)
    nthread :
        number of worker threads to use; if unspecified, use maximum number of
        hardware threads
    verbose :
        Whether to print extra messages during construction
    """

    def __init__(
        self,
        libpath: Union[str, pathlib.Path],
        *,
        nthread: Optional[int] = None,
        verbose: bool = False,
    ):
        self.handle = None

        nthread = nthread if nthread is not None else -1
        libpath = pathlib.Path(libpath).expanduser().resolve()
        if libpath.is_dir():
            # directory is given; locate shared library inside it
            resolved_libpath = None
            ext = _libext()
            for candidate in libpath.glob(f"*{ext}"):
                try:
                    resolved_libpath = candidate.resolve(strict=True)
                    break
                except FileNotFoundError:
                    continue
            if not resolved_libpath:
                raise TL2cgenError(
                    f"Directory {libpath} doesn't appear to have any dynamic shared library."
                )
        else:  # libpath is actually the name of shared library file
            if not libpath.exists():
                raise TL2cgenError(f"Shared library not found at location {libpath}")
            resolved_libpath = libpath

        self.handle = ctypes.c_void_p()
        _check_call(
            _LIB.TL2cgenPredictorLoad(
                c_str(str(resolved_libpath)),
                ctypes.c_int(nthread),
                ctypes.byref(self.handle),
            )
        )
        self._load_metadata(self.handle)

        if verbose:
            print(
                f"Dynamic shared library {resolved_libpath} has been successfully "
                "loaded into memory"
            )

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenPredictorFree(self.handle))
            self.handle = None

    @property
    def num_feature(self):
        """Query number of features used in the model"""
        return self.num_feature_

    @property
    def num_target(self):
        """Query number of output targets"""
        return self.num_target_

    @property
    def num_class(self):
        """Query number of class for each output target"""
        return self.num_class_

    @property
    def threshold_type(self):
        """Query threshold type of the model"""
        return self.threshold_type_

    @property
    def leaf_output_type(self):
        """Query threshold type of the model"""
        return self.leaf_output_type_

    def predict(
        self,
        dmat: DMatrix,
        *,
        verbose: bool = False,
        pred_margin: bool = False,
    ):
        """
        Perform batch prediction with a 2D sparse data matrix. Worker threads will
        internally divide up work for batch prediction. **Note that this function
        may be called by only one thread at a time.**

        Parameters
        ----------
        dmat:
            Batch of rows for which predictions will be made
        verbose :
            Whether to print extra messages during prediction
        pred_margin:
            Whether to produce raw margins rather than transformed probabilities
        """
        if not isinstance(dmat, DMatrix):
            raise TL2cgenError("dmat must be of type DMatrix")
        out_shape = ctypes.POINTER(ctypes.c_uint64)()
        out_ndim = ctypes.c_uint64()
        _check_call(
            _LIB.TL2cgenPredictorGetOutputShape(
                self.handle,
                dmat.handle,
                ctypes.byref(out_shape),
                ctypes.byref(out_ndim),
            )
        )
        output_shape = np.copy(
            np.ctypeslib.as_array(out_shape, shape=(out_ndim.value,)), order="C"
        )
        if self.leaf_output_type == "float32":
            output_array_dtype = np.float32
            output_array_cptr_type = ctypes.POINTER(ctypes.c_float)
        elif self.leaf_output_type == "float64":
            output_array_dtype = np.float64
            output_array_cptr_type = ctypes.POINTER(ctypes.c_double)  # type: ignore
        else:
            raise TL2cgenError(f"Unknown leaf_output_type {self.leaf_output_type}")

        output_array = np.zeros(shape=output_shape, dtype=output_array_dtype, order="C")
        _check_call(
            _LIB.TL2cgenPredictorPredictBatch(
                self.handle,
                dmat.handle,
                ctypes.c_int(1 if verbose else 0),
                ctypes.c_int(1 if pred_margin else 0),
                output_array.ctypes.data_as(output_array_cptr_type),
            )
        )
        return output_array

    def _load_metadata(self, handle: ctypes.c_void_p) -> None:
        num_feature = ctypes.c_int32()
        _check_call(
            _LIB.TL2cgenPredictorGetNumFeature(handle, ctypes.byref(num_feature))
        )
        self.num_feature_ = num_feature.value

        num_target = ctypes.c_int32()
        _check_call(_LIB.TL2cgenPredictorGetNumTarget(handle, ctypes.byref(num_target)))
        self.num_target_ = num_target.value

        num_class = np.zeros((self.num_target_,), dtype=np.int32)
        _check_call(
            _LIB.TL2cgenPredictorGetNumClass(
                handle, num_class.ctypes.data_as(ctypes.POINTER(ctypes.c_int32))
            )
        )
        self.num_class_ = num_class

        threshold_type = ctypes.c_char_p()
        _check_call(
            _LIB.TL2cgenPredictorGetThresholdType(handle, ctypes.byref(threshold_type))
        )
        self.threshold_type_ = py_str(threshold_type.value)

        leaf_output_type = ctypes.c_char_p()
        _check_call(
            _LIB.TL2cgenPredictorGetLeafOutputType(
                handle, ctypes.byref(leaf_output_type)
            )
        )
        self.leaf_output_type_ = py_str(leaf_output_type.value)
