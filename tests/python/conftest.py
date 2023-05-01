"""Pytest fixtures to initialize tests"""
import pathlib
import tempfile

import pytest
import treelite
from sklearn.datasets import load_svmlight_file

import tl2cgen

from .metadata import dataset_db


@pytest.fixture(scope="session")
def annotation():
    """Pre-computed branch annotation information for example datasets"""
    with tempfile.TemporaryDirectory(dir=".") as tmpdir:

        def compute_annotation(dataset):
            model = treelite.Model.load(
                dataset_db[dataset].model, model_format=dataset_db[dataset].format
            )
            if dataset_db[dataset].dtrain is None:
                return None
            dtrain = tl2cgen.DMatrix(
                load_svmlight_file(dataset_db[dataset].dtrain, zero_based=True)[0]
            )
            annotation_path = pathlib.Path(tmpdir) / f"{dataset}.json"
            tl2cgen.annotate_branch(
                path=annotation_path,
                model=model,
                dmat=dtrain,
                verbose=True,
            )
            with open(annotation_path, "r", encoding="utf-8") as f:
                return f.read()

        annotation_db = {k: compute_annotation(k) for k in dataset_db}
    return annotation_db
