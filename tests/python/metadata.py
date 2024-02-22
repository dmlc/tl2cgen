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
    "model format dtrain dtest libname dtype",
)

_dataset_db = {
    "mushroom": Dataset(
        model="mushroom.model",
        format="xgboost",
        dtrain="agaricus.train",
        dtest="agaricus.test",
        libname="agaricus",
        dtype="float32",
    ),
    "dermatology": Dataset(
        model="dermatology.model",
        format="xgboost",
        dtrain="dermatology.train",
        dtest="dermatology.test",
        libname="dermatology",
        dtype="float32",
    ),
    "toy_categorical": Dataset(
        model="toy_categorical_model.txt",
        format="lightgbm",
        dtrain=None,
        dtest="toy_categorical.test",
        libname="toycat",
        dtype="float64",
    ),
    "sparse_categorical": Dataset(
        model="sparse_categorical_model.txt",
        format="lightgbm",
        dtrain=None,
        dtest="sparse_categorical.test",
        libname="sparsecat",
        dtype="float64",
    ),
    "xgb_toy_categorical": Dataset(
        model="xgb_toy_categorical_model.json",
        format="xgboost_json",
        dtrain=None,
        dtest="xgb_toy_categorical.test",
        libname="xgbtoycat",
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
    if example_model_db[example_id].format == "xgboost":
        return treelite.frontend.load_xgboost_model_legacy_binary(
            example_model_db[example_id].model
        )
    if example_model_db[example_id].format == "xgboost_json":
        return treelite.frontend.load_xgboost_model(example_model_db[example_id].model)
    if example_model_db[example_id].format == "lightgbm":
        return treelite.frontend.load_lightgbm_model(example_model_db[example_id].model)
    raise ValueError("Unknown model format")


example_model_db = {
    k: v._replace(
        model=_qualify_path(k, v.model),
        dtrain=_qualify_path(k, v.dtrain),
        dtest=_qualify_path(k, v.dtest),
    )
    for k, v in _dataset_db.items()
}
