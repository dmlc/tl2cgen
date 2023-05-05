Deploying models with TL2cgen4J
================================

.. warning:: Java runtime currently not maintained

   Note. Currently, the Java runtime is not actively maintained.
   If you'd like to volunteer maintaining it, please e-mail chohyu01@cs.washington.edu.

TL2cgen4J is the Java runtime for TL2cgen. This tutorial will show how to use TL2cgen4J to deploy decision tree models to Java applications.

Build and install the Java runtime
----------------------------------
Check out the copy of TL2cgen by running

.. code-block:: console

  git clone https://github.com/dmlc/tl2cgen

Navigate to the ``java_runtime`` directory and run Maven to install the Java runtime:

.. code-block:: console

  cd java_runtime/tl2cgen4j
  mvn install

Specify TL2cgen4J as a dependency for your Java application
-----------------------------------------------------------
Add the following entry to your ``pom.xml``:

.. code-block:: xml

  <dependencies>
    <dependency>
      <groupId>ml.dmlc</groupId>
      <artifactId>tl2cgen4j</artifactId>
      <version>{latest_snapshot_version}</version>
    </dependency>
  </dependencies>

Load compiled model
-------------------
Locate the compiled model (dll/so/dylib) in the local filesystem. We load the compiled model by creating a Predictor object:

.. code-block:: java

  import ml.dmlc.tl2cgen4j.java.Predictor;

  Predictor predictor = new Predictor("path/to/compiled_model.so", -1, true);

The second argument is set to -1, to utilize all CPU cores available.

Query the model
---------------
Once the compiled model is loaded, we can query it:

.. code-block:: java

  // Get the input dimension, i.e. the number of feature values in the input vector
  int num_feature = predictor.GetNumFeature();

  // Get the number of classes.
  // This number is 1 for tasks other than multi-class classification.
  // For multi-class classification task, the number is equal to the number of classes.
  int num_class = predictor.GetNumClass();

Predict with a data matrix
--------------------------
For predicting with a batch of inputs, we create a list of DataPoint objects. Each DataPoint object consists of feature values and corresponding feature indices.

Let us look at an example. Consider the following 4-by-6 data matrix

.. math::

  \left[
    \begin{array}{cccccc}
      10 & 20 & \cdot & \cdot & \cdot & \cdot\\
      \cdot & 30 & \cdot & 40 & \cdot & \cdot\\
      \cdot & \cdot & 50 & 60 & 70 & \cdot\\
      \cdot & \cdot & \cdot & \cdot & \cdot & 80
    \end{array}
  \right]

where the dot (.) indicates the missing value. The matrix consists of 4 data points (instances), each with 6 feature values.
Since not all feature values are present, we need to store feature indices as well as feature values:

.. code-block:: java

  import ml.dmlc.tl2cgen4j.DataPoint;

  // Create a list consisting of 4 data points
  List<DataPoint> data_list = new ArrayList<DataPoint>() {
    {
      //                feature indices     feature values
      add(new DataPoint(new int[]{0, 1},    new float[]{10f, 20f}));
      add(new DataPoint(new int[]{1, 3},    new float[]{30f, 40f}));
      add(new DataPoint(new int[]{2, 3, 4}, new float[]{50f, 60f, 70f}));
      add(new DataPoint(new int[]{5},       new float[]{80f}));
    }
  };

Once the list is created, we then convert it into a DMatrix object. We use sparse representation here
because significant portion of the data matrix consists of missing values.

.. code-block:: java

  import ml.dmlc.tl2cgen4j.java.DMatrix;
  import ml.dmlc.tl2cgen4j.java.DMatrixBuilder;

  // Convert data point list into DMatrix object
  DMatrix dmat = DMatrixBuilder.createSparseCSRDMatrix(data_list.iterator());

Now invoke the dmat prediction function using the DMatrix object:

.. code-block:: java

  // verbose=true, pred_margin=false
  float[][] result = predictor.predict(dmat, true, false).toFloatMatrix();

The returned array is a two-dimensional array where the array ``result[i]`` represents the prediction for the ``i``-th data point. For most applications, each ``result[i]`` has length 1. Multi-class classification task is specical, in that for that task ``result[i]`` contains class probabilities, so the array is as long as the number of target classes.

For your convenience, we also provide a convenience method to load a data text file in the LIBSVM format:

.. code-block:: java

  List<DataPoint> data_list = DMatrixBuilder.LoadDatasetFromLibSVM("path/to/my.data.libsvm");
  DMatrix dmat = DMatrixBuilder.createSparseCSRDMatrix(data_list.iterator());
