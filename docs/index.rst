TL2cgen: model compiler for decision trees
==========================================

**TL2cgen** (TreeLite 2 C GENerator) is a model compiler for decision tree models.
You can convert any decision tree models
(random forests, gradient boosting models) into C code and distribute it as a native binary.

TL2cgen seamlessly integrates with `Treelite <https://treelite.readthedocs.io/en/latest>`_.
Any tree models that are supported by Treelite can be translated using TL2cgen.

***********
Quick start
***********
Install TL2cgen from PyPI:

.. code-block:: console

   pip install tl2cgen

Or from Conda-forge:

.. code-block:: console

   conda install -c conda-forge tl2cgen

*************************************
How are Treelite and TL2cgen related?
*************************************
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

  now an exchange and serialization format for decision
  tree models. Treelite  as a small library that's easy to embed in other C++
  applications.
* **TL2cgen** is now the model compiler, to convert decision tree models into C code.

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`