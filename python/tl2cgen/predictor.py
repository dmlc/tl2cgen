"""
Predictor module
"""
import ctypes
import pathlib
from typing import Optional, Union

from .contrib.util import _libext
from .data import DMatrix
from .exception import TL2cgenError
from .handle_class import _OutputVector
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
    def num_class(self):
        """Query number of output groups of the model"""
        return self.num_class_

    @property
    def pred_transform(self):
        """Query pred transform of the model"""
        return self.pred_transform_

    @property
    def global_bias(self):
        """Query global bias of the model"""
        return self.global_bias_

    @property
    def sigmoid_alpha(self):
        """Query sigmoid alpha of the model"""
        return self.sigmoid_alpha_

    @property
    def ratio_c(self):
        """Query sigmoid alpha of the model"""
        return self.ratio_c_

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
        result_size = ctypes.c_size_t()
        _check_call(
            _LIB.TL2cgenPredictorQueryResultSize(
                self.handle, dmat.handle, ctypes.byref(result_size)
            )
        )

        out_result = _OutputVector(
            predictor_handle=self.handle,
            dmat_handle=dmat.handle,
        )
        out_result_size = ctypes.c_size_t()
        _check_call(
            _LIB.TL2cgenPredictorPredictBatch(
                self.handle,
                dmat.handle,
                ctypes.c_int(1 if verbose else 0),
                ctypes.c_int(1 if pred_margin else 0),
                out_result.handle,
                ctypes.byref(out_result_size),
            )
        )

        out_result_array = out_result.toarray()
        idx = int(out_result_size.value)
        res = out_result_array[0:idx].reshape((dmat.shape[0], -1))
        if self.num_class_ > 1 and dmat.shape[0] != idx:
            res = res.reshape((-1, self.num_class_))

        return res

    def _load_metadata(self, handle: ctypes.c_void_p) -> None:
        # Save # of features
        num_feature = ctypes.c_size_t()
        _check_call(
            _LIB.TL2cgenPredictorQueryNumFeature(handle, ctypes.byref(num_feature))
        )
        self.num_feature_ = num_feature.value
        # Save # of classes
        num_class = ctypes.c_size_t()
        _check_call(_LIB.TL2cgenPredictorQueryNumClass(handle, ctypes.byref(num_class)))
        self.num_class_ = num_class.value
        # Save # of pred transform
        pred_transform = ctypes.c_char_p()
        _check_call(
            _LIB.TL2cgenPredictorQueryPredTransform(
                handle, ctypes.byref(pred_transform)
            )
        )
        self.pred_transform_ = py_str(pred_transform.value)
        # Save # of sigmoid alpha
        sigmoid_alpha = ctypes.c_float()
        _check_call(
            _LIB.TL2cgenPredictorQuerySigmoidAlpha(handle, ctypes.byref(sigmoid_alpha))
        )
        self.sigmoid_alpha_ = sigmoid_alpha.value
        # Save # of ratio C
        ratio_c = ctypes.c_float()
        _check_call(_LIB.TL2cgenPredictorQueryRatioC(handle, ctypes.byref(ratio_c)))
        self.ratio_c_ = ratio_c.value
        # Save # of global bias
        global_bias = ctypes.c_float()
        _check_call(
            _LIB.TL2cgenPredictorQueryGlobalBias(handle, ctypes.byref(global_bias))
        )
        self.global_bias_ = global_bias.value
        # Save threshold type
        threshold_type = ctypes.c_char_p()
        _check_call(
            _LIB.TL2cgenPredictorQueryThresholdType(
                handle, ctypes.byref(threshold_type)
            )
        )
        self.threshold_type_ = py_str(threshold_type.value)
        # Save leaf output type
        leaf_output_type = ctypes.c_char_p()
        _check_call(
            _LIB.TL2cgenPredictorQueryLeafOutputType(
                handle, ctypes.byref(leaf_output_type)
            )
        )
        self.leaf_output_type_ = py_str(leaf_output_type.value)
