"""Pytest fixtures to initialize tests"""

import pathlib
import tempfile

import pytest

import tl2cgen

from .metadata import example_model_db, load_example_model


@pytest.fixture(scope="session")
def annotation():
    """Pre-computed branch annotation information for example datasets"""
    try:
        from sklearn.datasets import load_svmlight_file
    except ImportError:
        pytest.skip(reason="scikit-learn is required")

    with tempfile.TemporaryDirectory(dir=".") as tmpdir:

        def compute_annotation(dataset):
            model = load_example_model(dataset)
            if example_model_db[dataset].dtrain is None:
                return None
            dtrain = tl2cgen.DMatrix(
                load_svmlight_file(example_model_db[dataset].dtrain, zero_based=True)[0]
            )
            annotation_path = pathlib.Path(tmpdir) / f"{dataset}.json"
            tl2cgen.annotate_branch(
                model=model,
                dmat=dtrain,
                path=annotation_path,
                verbose=True,
            )
            with open(annotation_path, "r", encoding="utf-8") as f:
                return f.read()

        annotation_db = {k: compute_annotation(k) for k in example_model_db}
    return annotation_db
