"""Metadata for datasets and models used for testing"""

import collections
import pathlib
from typing import Optional, Union

import treelite

from tl2cgen.contrib.util import _libext

CURRENT_DIR = pathlib.Path(__file__).parent.expanduser().resolve()
DPATH = CURRENT_DIR.parent / "example_data"

Dataset = collections.namedtuple(
    "Dataset",
    "model format dtrain dtest libname expected_prob expected_margin is_multiclass dtype",
)

_dataset_db = {
    "mushroom": Dataset(
        model="mushroom.model",
        format="xgboost",
        dtrain="agaricus.train",
        dtest="agaricus.test",
        libname="agaricus",
        expected_prob="agaricus.test.prob",
        expected_margin="agaricus.test.margin",
        is_multiclass=False,
        dtype="float32",
    ),
    "dermatology": Dataset(
        model="dermatology.model",
        format="xgboost",
        dtrain="dermatology.train",
        dtest="dermatology.test",
        libname="dermatology",
        expected_prob="dermatology.test.prob",
        expected_margin="dermatology.test.margin",
        is_multiclass=True,
        dtype="float32",
    ),
    "toy_categorical": Dataset(
        model="toy_categorical_model.txt",
        format="lightgbm",
        dtrain=None,
        dtest="toy_categorical.test",
        libname="toycat",
        expected_prob=None,
        expected_margin="toy_categorical.test.pred",
        is_multiclass=False,
        dtype="float64",
    ),
    "sparse_categorical": Dataset(
        model="sparse_categorical_model.txt",
        format="lightgbm",
        dtrain=None,
        dtest="sparse_categorical.test",
        libname="sparsecat",
        expected_prob=None,
        expected_margin="sparse_categorical.test.margin",
        is_multiclass=False,
        dtype="float64",
    ),
    "xgb_toy_categorical": Dataset(
        model="xgb_toy_categorical_model.json",
        format="xgboost_json",
        dtrain=None,
        dtest="xgb_toy_categorical.test",
        libname="xgbtoycat",
        expected_prob=None,
        expected_margin="xgb_toy_categorical.test.pred",
        is_multiclass=False,
        dtype="float32",
    ),
}


def _qualify_path(prefix: str, path: Optional[str]) -> Optional[str]:
    if path is None:
        return None
    new_path = DPATH / prefix / path
    assert new_path.exists()
    return str(new_path)


def format_libpath_for_example_model(
    example_id: str,
    prefix: Union[pathlib.Path, str],
) -> pathlib.Path:
    """Format path for the shared lib corresponding to an example model"""
    prefix = pathlib.Path(prefix).expanduser().resolve()
    return prefix.joinpath(example_model_db[example_id].libname + _libext())


def load_example_model(example_id: str) -> treelite.Model:
    """Load an example model"""
    return treelite.Model.load(
        example_model_db[example_id].model,
        model_format=example_model_db[example_id].format,
    )


example_model_db = {
    k: v._replace(
        model=_qualify_path(k, v.model),
        dtrain=_qualify_path(k, v.dtrain),
        dtest=_qualify_path(k, v.dtest),
        expected_prob=_qualify_path(k, v.expected_prob),
        expected_margin=_qualify_path(k, v.expected_margin),
    )
    for k, v in _dataset_db.items()
}
