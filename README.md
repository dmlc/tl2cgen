# TL2cgen

[![Documentation Status](https://readthedocs.org/projects/tl2cgen/badge/?version=latest)](http://tl2cgen.readthedocs.io/en/latest/?badge=latest)
[![codecov](https://codecov.io/gh/dmlc/tl2cgen/branch/main/graph/badge.svg)](https://codecov.io/gh/dmlc/tl2cgen)
[![GitHub license](http://dmlc.github.io/img/apache2.svg)](./LICENSE)
[![PyPI version](https://badge.fury.io/py/tl2cgen.svg)](https://pypi.python.org/pypi/tl2cgen/)
[![Conda Version](https://img.shields.io/conda/vn/conda-forge/tl2cgen.svg)](https://anaconda.org/conda-forge/tl2cgen)

**TL2cgen** is a model compiler for decision tree models.
You can convert any decision tree models (random forests, gradient boosting models) into C code and distribute it as a native binary.

TL2cgen seamlessly integrates with [Treelite](https://github.com/dmlc/treelite).
Any tree models that are supported by Treelite can be translated using TL2cgen.

## How are Treelite and TL2cgen related?

See [this page](https://tl2cgen.readthedocs.io/en/latest/#how-are-treelite-and-tl2cgen-related).
