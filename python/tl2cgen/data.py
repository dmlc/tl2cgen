"""Data matrix"""
import ctypes
from typing import Optional, Tuple, Union

import numpy as np
import numpy.typing as npt
import scipy  # type: ignore

from .dtypes import (
    numpy_type_to_type_info,
    type_info_to_ctypes_type,
    type_info_to_numpy_type,
)
from .exception import TL2cgenError
from .libloader import _LIB, _check_call
from .util import c_str


class DMatrix:
    """Data matrix used in TL2cgen.

    Parameters
    ----------
    data :
        Data source
    dtype :
        If specified, the data will be casted into the corresponding data type.
    missing :
        Value in the data that represents a missing entry. If set to ``None``,
        ``numpy.nan`` will be used.
    """

    # pylint: disable=R0902,R0903,R0913

    def __init__(
        self,
        data: Union[str, npt.NDArray, scipy.sparse.csr_matrix],
        *,
        dtype: Optional[str] = None,
        missing: Optional[float] = None,
    ):
        if data is None:
            raise TL2cgenError("'data' argument cannot be None")

        self.handle = ctypes.c_void_p()

        if isinstance(data, (str,)):
            raise TL2cgenError(
                "'data' argument cannot be a string. Did you mean to load data from a text file? "
                "Please use the following packages to load the text file:\n"
                "   * CSV file: Use pandas.read_csv() or numpy.loadtxt()\n"
                "   * LIBSVM file: Use sklearn.datasets.load_svmlight_file()"
            )
        if isinstance(data, scipy.sparse.csr_matrix):
            self._init_from_csr(data, dtype=dtype)
        elif isinstance(data, scipy.sparse.csc_matrix):
            self._init_from_csr(data.tocsr(), dtype=dtype)
        elif isinstance(data, np.ndarray):
            self._init_from_npy2d(data, missing=missing, dtype=dtype)
        else:  # any type that's convertible to CSR matrix is O.K.
            try:
                csr = scipy.sparse.csr_matrix(data)
                self._init_from_csr(csr, dtype=dtype)
            except Exception as e:
                raise TypeError(
                    f"Cannot initialize DMatrix from {type(data).__name__}"
                ) from e
        num_row, num_col, nelem = self._get_dims()
        self.shape = (num_row, num_col)
        self.size = nelem

    def _init_from_csr(
        self, csr: scipy.sparse.csr_matrix, *, dtype: Optional[str] = None
    ) -> None:
        """Initialize data from a CSR (Compressed Sparse Row) matrix"""
        if len(csr.indices) != len(csr.data):
            raise ValueError(
                f"indices and data not of same length: {len(csr.indices)} vs {len(csr.data)}"
            )
        if len(csr.indptr) != csr.shape[0] + 1:
            raise ValueError(
                "len(indptr) must be equal to 1 + [number of rows]"
                f"len(indptr) = {len(csr.indptr)} vs 1 + [number of rows] = {1 + csr.shape[0]}"
            )
        if csr.indptr[-1] != len(csr.data):
            raise ValueError(
                "last entry of indptr must be equal to len(data)"
                f"indptr[-1] = {csr.indptr[-1]} vs len(data) = {len(csr.data)}"
            )

        if dtype is None:
            data_type = csr.data.dtype
        else:
            data_type = type_info_to_numpy_type(dtype)
        data_type_code = numpy_type_to_type_info(data_type)
        data_ptr_type = ctypes.POINTER(type_info_to_ctypes_type(data_type_code))
        if data_type_code not in ["float32", "float64"]:
            raise ValueError("data should be either float32 or float64 type")

        data = np.array(csr.data, copy=False, dtype=data_type, order="C")
        indices = np.array(csr.indices, copy=False, dtype=np.uintc, order="C")
        indptr = np.array(csr.indptr, copy=False, dtype=np.uintp, order="C")
        _check_call(
            _LIB.TL2cgenDMatrixCreateFromCSR(
                data.ctypes.data_as(data_ptr_type),
                c_str(data_type_code),
                indices.ctypes.data_as(ctypes.POINTER(ctypes.c_uint)),
                indptr.ctypes.data_as(ctypes.POINTER(ctypes.c_size_t)),
                ctypes.c_size_t(csr.shape[0]),
                ctypes.c_size_t(csr.shape[1]),
                ctypes.byref(self.handle),
            )
        )

    def _init_from_npy2d(
        self,
        mat: npt.NDArray,
        *,
        missing: Optional[float] = None,
        dtype: Optional[str] = None,
    ) -> None:
        """
        Initialize data from a 2-D numpy matrix.
        If ``mat`` does not have ``order='C'`` (also known as row-major) or is not
        contiguous, a temporary copy will be made.
        If ``mat`` does not have ``dtype=numpy.float32``, a temporary copy will be
        made also.
        Thus, as many as two temporary copies of data can be made. One should set
        input layout and type judiciously to conserve memory.
        """
        if len(mat.shape) != 2:
            raise ValueError("Input numpy.ndarray must be two-dimensional")
        data_type: npt.DTypeLike = (
            mat.dtype if dtype is None else type_info_to_numpy_type(dtype)
        )
        data_type_code = numpy_type_to_type_info(data_type)
        data_ptr_type = ctypes.POINTER(type_info_to_ctypes_type(data_type_code))
        if data_type_code not in ["float32", "float64"]:
            raise ValueError("data should be either float32 or float64 type")
        # flatten the array by rows and ensure it is float32.
        # we try to avoid data copies if possible
        # (reshape returns a view when possible and we explicitly tell np.array to
        #  avoid copying)
        data = np.array(mat.reshape(mat.size), copy=False, dtype=data_type)
        missing = missing if missing is not None else np.nan
        missing_ar = np.array([missing], dtype=data_type, order="C")
        _check_call(
            _LIB.TL2cgenDMatrixCreateFromMat(
                data.ctypes.data_as(data_ptr_type),
                c_str(data_type_code),
                ctypes.c_size_t(mat.shape[0]),
                ctypes.c_size_t(mat.shape[1]),
                missing_ar.ctypes.data_as(data_ptr_type),
                ctypes.byref(self.handle),
            )
        )

    def _get_dims(self) -> Tuple[int, int, int]:
        num_row = ctypes.c_size_t()
        num_col = ctypes.c_size_t()
        nelem = ctypes.c_size_t()
        _check_call(
            _LIB.TL2cgenDMatrixGetDimension(
                self.handle,
                ctypes.byref(num_row),
                ctypes.byref(num_col),
                ctypes.byref(nelem),
            )
        )
        return num_row.value, num_col.value, nelem.value

    def __del__(self):
        if self.handle:
            _check_call(_LIB.TL2cgenDMatrixFree(self.handle))
            self.handle = None

    def __repr__(self):
        return (
            f"<{self.shape[0]}x{self.shape[1]} sparse matrix of type tl2cgen.DMatrix\n"
            f"        with {self.size} stored elements in Compressed Sparse Row format>"
        )
