==========================================
TL2cgen: model compiler for decision trees
==========================================

**TL2cgen** (TreeLite 2 C GENerator) is a model compiler for decision tree models.
You can convert any decision tree models
(random forests, gradient boosting models) into C code and distribute it as a native binary.

TL2cgen seamlessly integrates with `Treelite <https://treelite.readthedocs.io/en/latest>`_.
Any tree models that are supported by Treelite can be converted into C via TL2cgen.

Quick start
===========

Install TL2cgen from PyPI:

.. code-block:: console

   pip install tl2cgen

Or from Conda-forge:

.. code-block:: console

   conda install -c conda-forge tl2cgen

To use TL2cgen, first import your tree ensemble model:

.. code-block:: python

  import treelite  # Used for importing tree model
  model = treelite.Model.load("my_model.json", model_format="xgboost_json")

Now use TL2cgen to generate C code:

.. code-block:: python

  import tl2cgen
  tl2cgen.generate_c_code(model, dirpath="./src")

TL2cgen also provides a convenient wrapper for building native shared libraries:

.. code-block:: python

  tl2cgen.export_lib(model, toolchain="gcc", libpath="./predictor.so")

You can also build a source archive, with a CMakeLists.txt:

.. code-block:: python

  # Run `cmake` on the target machine
  tl2cgen.export_srcpkg(model, toolchain="cmake", pkgpath="./archive.zip",
                        libname="predictor")

Finally, make predictions with the native library using the :py:class:`~tl2cgen.Predictor`
class:

.. code-block:: python

   predictor = tl2cgen.Predictor("./predictor.so")
   dmat = tl2cgen.DMatrix(X)
   out_pred = predictor.predict(dmat)

How are Treelite and TL2cgen related?
=====================================

Starting from 4.0 version, Treelite no longer supports compiling tree models into
C code. That part of Treelite has been migrated to TL2cgen.

* **Treelite** (starting from 4.0) is now a small libary that enables other C++
  applications to exchange and store decision trees on the disk as well as the
  network. By using Treelite, application writers can support many kinds of
  tree models with minimal code duplication and high level of correctness.
  Trees are stored using an efficient binary format.
* **TL2cgen** is a model compiler that converts tree models into C code.
  It can convert any tree models that can be stored using the Treelite format.
  TL2cgen is one of multiple applications that uses Treelite as a library.


Contents
========

.. toctree::
  :maxdepth: 2
  :titlesonly:

  api
  compiler_param
  tl2cgen-doxygen

Indices
=======

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
