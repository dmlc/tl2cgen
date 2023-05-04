=================================================
Migration Guide: How to migrate from Treelite 3.x
=================================================

Starting from 4.0 version, Treelite no longer supports compiling tree models into
C code. That part of Treelite has been migrated to TL2cgen.

In this guide, we will show you how to migrate your code to use TL2cgen instead of
Treelite version 3.x.

.. contents::
  :local:

Model loading
=============
The logic for loading tree model into memory remains identical. Keep using Treelite
for model loading.

.. code-block:: python
  :caption: Load XGBoost model from files

  model = treelite.Model.load("xgboost_model.bin", model_format="xgboost")
  # Or
  model = treelite.Model.load("xgboost_model.json", model_format="xgboost_json")
  # Or
  model = treelite.Model.from_xgboost(bst)  # bst is xgboost.Booster type

.. code-block:: python
  :caption: Load LightGBM model

  model = treelite.Model.load("lightgbm_model.txt", model_format="lightgbm")
  # Or
  model = treelite.Model.from_lightgbm(bst)  # bst is lightgbm.Booster type

.. code-block:: python
  :caption: Load scikit-learn model

  model = treelite.sklearn.import_model(clf)  # clf is a scikit-learn model object


.. code-block:: python
  :caption: Load a tree model using the model builder interface

  builder = treelite.ModelBuilder(num_feature=3)

  tree = treelite.ModelBuilder.Tree()
  tree[0].set_numerical_test_node(
    feature_id=0,
    opname="<",
    threshold=5.0,
    default_left=True,
    left_child_key=1,
    right_child_key=2
  )
  tree[1].set_leaf_node(0.6)
  tree[2].set_leaf_node(-0.4)
  tree[0].set_root()

  builder.append(tree)
  model = builder.commit()  # Obtain treelite.Model object

Generating C code
=================

For generating C code from tree models, replace ``treelite.Model.compile()``
with :py:func:`tl2cgen.generate_c_code`

.. code-block:: python

  # Before: Treelite 3.x
  model.compile(dirpath="./code_dir", params={})

  # After: TL2cgen. Note the model object being passed as the first argument
  tl2cgen.generate_c_code(model, dirpath="./code_dir", params={})

Exporting libraries
===================
Replace ``treelite.Model.export_lib()`` with :py:func:`tl2cgen.export_lib`:

.. code-block:: python

  # Before: Treelite 3.x
  model.export_lib(toolchain="msvc", libpath="./mymodel.dll", params={})

  # After: TL2cgen. The model object is passed as the first argument
  tl2cgen.export_lib(model, toolchain="msvc", libpath="./mymodel.dll", params={})

``treelite.Model.export_srcpkg()`` is replaced with :py:func:`tl2cgen.export_srcpkg`.
Note that the parameter ``platform`` was removed in :py:func:`tl2cgen.export_srcpkg`.

.. code-block:: python

  # Before: Treelite 3.x
  model.export_srcpkg(platform="unix", toolchain="gcc", pkgpath="./mymodel_pkg.zip",
                      libname="mymodel.so", params={})

  # After: TL2cgen. The model object is passed as the first argument
  # 'platform' parameter is removed.
  tl2cgen.export_srcpkg(model, toolchain="gcc", pkgpath="./mymodel_pkg.zip",
                        libname="mymodel.so", params={})

Predicting with exported libraries
==================================
Replace ``treelite_runtime.Predictor`` class with :py:class:`tl2cgen.Predictor`.
In TL2cgen, the Predictor class is part of the same Python module as other
classes and methods; there is no separate "runtime" module.

In addition, ``treelite_runtime.DMatrix`` is replaced with
:py:class:`tl2cgen.DMatrix`.

.. code-block:: python

  # Before: Treelite 3.x
  predictor = treelite_runtime.Predictor("./mymodel.so")
  dmat = treelite_runtime.DMatrix(X)
  out_pred = predictor.predict(dmat)

  # After: TL2cgen
  predictor = tl2cgen.Predictor("./mymodel.so")
  dmat = tl2cgen.DMatrix(X)
  out_pred = predictor.predict(dmat)

Annotating branches
===================
Replace ``treelite.Annotator`` with :py:func:`tl2cgen.annotate_branch`.
Instead of calling two methods ``treelite.Annotator.annotate_branch`` and
``treelite.Annotator.save``, you only need to call one,
:py:func:`tl2cgen.annotate_branch`:

.. code-block:: python

  # Before: Treelite 3.x
  dmat = treelite_runtime.DMatrix(X_train)
  annotator = treelite.Annotator()
  annotator.annotate_branch(model, dmat)
  annotator.save(path="mymodel-annotation.json")

  # After: TL2cgen. Only one method call is needed.
  dmat = tl2cgen.DMatrix(X_train)
  tl2cgen.annotate_branch(model, dmat, path="mymodel-annotation.json")
