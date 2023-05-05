==============
First tutorial
==============

This tutorial will demonstrate the basic workflow.

.. code-block:: python

    import treelite
    import tl2cgen

Classification Example
======================

In this tutorial, we will use a small classification example to describe
the full workflow.

Load the Boston house prices dataset
------------------------------------

Let us use the Iris dataset from scikit-learn
(:py:func:`sklearn.datasets.load_iris`). It consists of 150 samples
with 4 distinct features:

.. code-block:: python

    from sklearn.datasets import load_iris
    X, y = load_iris(return_X_y=True)
    print(f"dimensions of X = {X.shape}")
    print(f"dimensions of y = {y.shape}")

Train a tree ensemble model using XGBoost
-----------------------------------------

The first step is to train a tree ensemble model using XGBoost
(`dmlc/xgboost <https://github.com/dmlc/xgboost/>`_).

Disclaimer: TL2cgen does NOT depend on the XGBoost package in any way.
XGBoost was used here only to provide a working example.

.. code-block:: python

    import xgboost as xgb
    dtrain = xgb.DMatrix(X, label=y)
    params = {"max_depth": 3, "eta": 0.1, "objective": "multi:softprob",
              "eval_metric": "mlogloss", "num_class": 3}
    bst = xgb.train(params, dtrain, num_boost_round=20,
                    evals=[(dtrain, 'train')])

Pass XGBoost model into Treelite
--------------------------------

Next, we feed the trained model into Treelite. If you used XGBoost to
train the model, it takes only one line of code:

.. code-block:: python

    model = treelite.Model.from_xgboost(bst)

.. note:: Using other packages to train decision trees

  With additional work, you can use models trained with other machine learning
  packages. See :doc:`this page <treelite:tutorials/import>` for instructions.

Generate shared library
-----------------------

Given a tree ensemble model, TL2cgen will produce a **prediction function**
in C. To use the function for prediction, we package it as a `dynamic shared
library <https://en.wikipedia.org/wiki/Library_(computing)#Shared_libraries>`_,
which exports the prediction function for other programs to use.

Before proceeding, you should decide which of the following compilers is
available on your system and set the variable ``toolchain`` appropriately:

-  ``gcc``
-  ``clang``
-  ``msvc`` (Microsoft Visual C++)

.. code-block:: python

    toolchain = "gcc"   # change this value as necessary

The choice of toolchain will be used to compile the prediction function
into native library.

Now we are ready to generate the library.

.. code-block:: python

    tl2cgen.export_lib(model, toolchain=toolchain, libpath="./mymodel.so")
      #                                                               ^^
      #       Set correct file extension here; see the following paragraph

.. note:: File extension for shared library

  Make sure to use the correct file extension for the library,
  depending on the operating system:

  -  Windows: ``.dll``
  -  Mac OS X: ``.dylib``
  -  Linux / Other UNIX: ``.so``

.. note:: Want to deploy the model to another machine?

  This tutorial assumes that predictions will be made on the same machine that
  is running Treelite. If you'd like to deploy your model to another machine,
  see the page :doc:`deploy`.

.. note:: Reducing compilation time for large models

  For large models, :py:meth:`tl2cgen.export_lib` may take a long time
  to finish. To reduce compilation time, enable the ``parallel_comp`` option by
  writing

  .. code-block:: python

    tl2cgen.export_lib(model, toolchain=toolchain, libpath="./mymodel.so",
                       params={"parallel_comp": 32})

  which splits the prediction subroutine into 32 source files that gets compiled
  in parallel. Adjust this number according to the number of cores on your
  machine.

Use the shared library to make predictions
------------------------------------------

Once the shared library has been generated, we can use it using
the :py:class:`tl2cgen.Predictor` class:

.. code-block:: python

    predictor = tl2cgen.Predictor("./mymodel.so")

We decide on which of the samples in ``X`` we should make predictions
for. Say, from 10th sample to 20th:

.. code-block:: python

    dmat = tl2cgen.DMatrix(X[10:20, :])
    out_pred = predictor.predict(dmat)
    print(out_pred)
