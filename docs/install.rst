==================
Installation Guide
==================

You may choose one of two methods to install TL2cgen on your system:

.. contents::
  :local:
  :depth: 1

.. _install-pip:

Download binary releases from PyPI (Recommended)
================================================
This is probably the most convenient method. Simply type

.. code-block:: console

  pip install tl2cgen

to install the TL2cgen package. The command will locate the binary release that is compatible with
your current platform. Check the installation by running

.. code-block:: python

  import tl2cgen

in an interactive Python session. This method is available for only Windows, MacOS, and Linux.
For other operating systems, see the next section.

.. note:: Windows users need to install Visual C++ Redistributable

  TL2cgen requires DLLs from `Visual C++ Redistributable
  <https://www.microsoft.com/en-us/download/details.aspx?id=48145>`_
  in order to function, so make sure to install it. Exception: If
  you have Visual Studio installed, you already have access to
  necessary libraries and thus don't need to install Visual C++
  Redistributable.

.. note:: Installing OpenMP runtime on MacOS

  TL2cgen requires the presence of OpenMP runtime. To install OpenMP runtime on a Mac OSX system,
  run the following command:

  .. code-block:: bash

    brew install libomp


.. _install-conda:

Download binary releases from Conda
===================================

TL2cgen is also available on Conda.

.. code-block:: console

  conda install -c conda-forge tl2cgen

to install the TL2cgen package. See https://anaconda.org/conda-forge/tl2cgen to check the
available platforms.

.. _install-source:

Compile TL2cgen from the source
===============================
Installation consists of two steps:

1. Build the native library from C++ code.
2. Install the Python package.

.. note:: Name of the native library

   ================== =====================
   Operating System   Shared library
   ================== =====================
   Windows            ``tl2cgen.dll``
   Mac OS X           ``libtl2cgen.dylib``
   Linux / other UNIX ``libtl2cgen.so``
   ================== =====================

To get started, clone TL2cgen repo from GitHub.

.. code-block:: bash

  git clone https://github.com/dmlc/tl2cgen.git
  cd tl2cgen

The next step is to build the native library.

1-1. Compiling the native library on Linux and MacOS
----------------------------------------------------
Here, we use CMake to generate a Makefile:

.. code-block:: bash

  mkdir build
  cd build
  cmake ..

Once CMake finished running, simply invoke GNU Make to obtain the native
libraries.

.. code-block:: bash

  make

The compiled library will be under the ``build/`` directory.

.. note:: Compiling TL2cgen with multithreading on MacOS

  TL2cgen requires the presence of OpenMP runtime. To install OpenMP runtime on a MacOS system,
  run the following command:

  .. code-block:: bash

    brew install libomp

1-2. Compiling native libraries on Windows
------------------------------------------
We can use CMake to generate a Visual Studio project. The following snippet assumes that Visual
Studio 2022 is installed. Adjust the version depending on the copy that's installed on your system.

.. code-block:: dosbatch

  mkdir build
  cd build
  cmake .. -G"Visual Studio 17 2022" -A x64

.. note:: Visual Studio 2019 or newer is required

  TL2cgen uses the C++17 standard. Ensure that you have Visual Studio version 2019 or newer.

Once CMake finished running, open the generated solution file (``tl2cgen.sln``) in Visual Studio.
From the top menu, select **Build > Build Solution**.

2. Installing Python package
----------------------------
The Python package is located at the ``python`` subdirectory. Run Pip to install the Python
package. The Python package will re-use the native library built in Step 1.

.. code-block:: bash

  cd python
  pip install .  # will re-use libtl2cgen.so
