package ml.dmlc.tl2cgen4j.java;

import org.apache.commons.logging.Log;
import org.apache.commons.logging.LogFactory;

/**
 * TL2cgen prediction runtime JNI functions
 * @author Hyunsu Cho
 */
class TL2cgenJNI {
  private static final Log logger = LogFactory.getLog(TL2cgenJNI.class);
  static {
    try {
      NativeLibLoader.initTL2cgenRuntime();
    } catch (Exception ex) {
      logger.error("Failed to load native library", ex);
      throw new RuntimeException(ex);
    }
  }

  /**
   * Check the return code of the JNI call.
   *
   * @throws TL2cgenError if the call failed.
   */
  static void checkCall(int ret) throws TL2cgenError {
    if (ret != 0) {
      throw new TL2cgenError(TL2cgenGetLastError());
    }
  }

  public static native String TL2cgenGetLastError();

  public static native int TL2cgenDMatrixCreateFromCSRWithFloat32In(
      float[] data, int[] col_ind, long[] row_ptr, long num_row, long num_col, long[] out);

  public static native int TL2cgenDMatrixCreateFromCSRWithFloat64In(
      double[] data, int[] col_ind, long[] row_ptr, long num_row, long num_col, long[] out);

  public static native int TL2cgenDMatrixCreateFromMatWithFloat32In(
      float[] data, long num_row, long num_col, float missing_value, long[] out);

  public static native int TL2cgenDMatrixCreateFromMatWithFloat64In(
      double[] data, long num_row, long num_col, double missing_value, long[] out);

  public static native int TL2cgenDMatrixGetDimension(
      long handle, long[] out_num_row, long[] out_num_col, long[] out_nelem);

  public static native int TL2cgenDMatrixFree(
      long handle);

  public static native int TL2cgenPredictorLoad(
      String library_path, int num_worker_thread, long[] out);

  public static native int TL2cgenPredictorPredictBatchWithFloat32Out(
      long handle, long batch, boolean verbose, boolean pred_margin, float[] out_result,
      long[] out_result_size);

  public static native int TL2cgenPredictorPredictBatchWithFloat64Out(
      long handle, long batch, boolean verbose, boolean pred_margin, double[] out_result,
      long[] out_result_size);

  public static native int TL2cgenPredictorPredictBatchWithUInt32Out(
      long handle, long batch, boolean verbose, boolean pred_margin, int[] out_result,
      long[] out_result_size);

  public static native int TL2cgenPredictorQueryResultSize(
      long handle, long batch, long[] out);

  public static native int TL2cgenPredictorQueryNumClass(
      long handle, long[] out);

  public static native int TL2cgenPredictorQueryNumFeature(
      long handle, long[] out);

  public static native int TL2cgenPredictorQueryPredTransform(
      long handle, String[] out);

  public static native int TL2cgenPredictorQuerySigmoidAlpha(
      long handle, float[] out);

  public static native int TL2cgenPredictorQueryRatioC(
      long handle, float[] out);

  public static native int TL2cgenPredictorQueryGlobalBias(
      long handle, float[] out);

  public static native int TL2cgenPredictorQueryThresholdType(
      long handle, String[] out);

  public static native int TL2cgenPredictorQueryLeafOutputType(
      long handle, String[] out);

  public static native int TL2cgenPredictorFree(long handle);

}
