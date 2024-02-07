/**
  Copyright (c) 2023 by Contributors
  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at
  http://www.apache.org/licenses/LICENSE-2.0
  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "./tl2cgen4j.h"

#include <tl2cgen/c_api.h>
#include <tl2cgen/c_api_error.h>
#include <tl2cgen/predictor.h>

#include <algorithm>
#include <cstring>
#include <limits>
#include <vector>

namespace {

// set handle
void setHandle(JNIEnv* jenv, jlongArray jhandle, void* handle) {
#ifdef __APPLE__
  auto out = static_cast<jlong>(reinterpret_cast<long>(handle));  // NOLINT(runtime/int)
#else
  auto out = reinterpret_cast<int64_t>(handle);
#endif
  jenv->SetLongArrayRegion(jhandle, 0, 1, &out);
}

}  // namespace

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenGetLastError
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenGetLastError(
    JNIEnv* jenv, jclass jcls) {
  jstring jresult = nullptr;
  char const* result = TL2cgenGetLastError();
  if (result) {
    jresult = jenv->NewStringUTF(result);
  }
  return jresult;
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixCreateFromCSRWithFloat32In
 * Signature: ([F[I[JJJ[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixCreateFromCSRWithFloat32In(JNIEnv* jenv,
    jclass jcls, jfloatArray jdata, jintArray jcol_ind, jlongArray jrow_ptr, jlong jnum_row,
    jlong jnum_col, jlongArray jout) {
  jfloat* data = jenv->GetFloatArrayElements(jdata, nullptr);
  jint* col_ind = jenv->GetIntArrayElements(jcol_ind, nullptr);
  jlong* row_ptr = jenv->GetLongArrayElements(jrow_ptr, nullptr);
  TL2cgenDMatrixHandle out = nullptr;
  int const ret = TL2cgenDMatrixCreateFromCSR(static_cast<void const*>(data), "float32",
      reinterpret_cast<uint32_t const*>(col_ind), reinterpret_cast<size_t const*>(row_ptr),
      static_cast<size_t>(jnum_row), static_cast<size_t>(jnum_col), &out);
  setHandle(jenv, jout, out);
  // release arrays
  jenv->ReleaseFloatArrayElements(jdata, data, 0);
  jenv->ReleaseIntArrayElements(jcol_ind, col_ind, 0);
  jenv->ReleaseLongArrayElements(jrow_ptr, row_ptr, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixCreateFromCSRWithFloat64In
 * Signature: ([D[I[JJJ[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixCreateFromCSRWithFloat64In(JNIEnv* jenv,
    jclass jcls, jdoubleArray jdata, jintArray jcol_ind, jlongArray jrow_ptr, jlong jnum_row,
    jlong jnum_col, jlongArray jout) {
  jdouble* data = jenv->GetDoubleArrayElements(jdata, nullptr);
  jint* col_ind = jenv->GetIntArrayElements(jcol_ind, nullptr);
  jlong* row_ptr = jenv->GetLongArrayElements(jrow_ptr, nullptr);
  TL2cgenDMatrixHandle out = nullptr;
  int const ret = TL2cgenDMatrixCreateFromCSR(static_cast<void const*>(data), "float64",
      reinterpret_cast<uint32_t const*>(col_ind), reinterpret_cast<size_t const*>(row_ptr),
      static_cast<size_t>(jnum_row), static_cast<size_t>(jnum_col), &out);
  setHandle(jenv, jout, out);
  // release arrays
  jenv->ReleaseDoubleArrayElements(jdata, data, 0);
  jenv->ReleaseIntArrayElements(jcol_ind, col_ind, 0);
  jenv->ReleaseLongArrayElements(jrow_ptr, row_ptr, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixCreateFromMatWithFloat32In
 * Signature: ([FJJF[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixCreateFromMatWithFloat32In(JNIEnv* jenv,
    jclass jcls, jfloatArray jdata, jlong jnum_row, jlong jnum_col, jfloat jmissing_value,
    jlongArray jout) {
  jfloat* data = jenv->GetFloatArrayElements(jdata, nullptr);
  float missing_value = static_cast<float>(jmissing_value);
  TL2cgenDMatrixHandle out = nullptr;
  int const ret = TL2cgenDMatrixCreateFromMat(static_cast<void const*>(data), "float32",
      static_cast<size_t>(jnum_row), static_cast<size_t>(jnum_col), &missing_value, &out);
  setHandle(jenv, jout, out);
  // release arrays
  jenv->ReleaseFloatArrayElements(jdata, data, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixCreateFromMatWithFloat64In
 * Signature: ([DJJD[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixCreateFromMatWithFloat64In(JNIEnv* jenv,
    jclass jcls, jdoubleArray jdata, jlong jnum_row, jlong jnum_col, jdouble jmissing_value,
    jlongArray jout) {
  jdouble* data = jenv->GetDoubleArrayElements(jdata, nullptr);
  double missing_value = static_cast<double>(jmissing_value);
  TL2cgenDMatrixHandle out = nullptr;
  int const ret = TL2cgenDMatrixCreateFromMat(static_cast<void const*>(data), "float64",
      static_cast<size_t>(jnum_row), static_cast<size_t>(jnum_col), &missing_value, &out);
  setHandle(jenv, jout, out);
  // release arrays
  jenv->ReleaseDoubleArrayElements(jdata, data, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixGetDimension
 * Signature: (J[J[J[J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixGetDimension(
    JNIEnv* jenv, jclass jcls, jlong jmat, jlongArray jout_num_row, jlongArray jout_num_col,
    jlongArray jout_nelem) {
  TL2cgenDMatrixHandle dmat = reinterpret_cast<TL2cgenDMatrixHandle>(jmat);
  size_t num_row = 0, num_col = 0, num_elem = 0;
  int const ret = TL2cgenDMatrixGetDimension(dmat, &num_row, &num_col, &num_elem);
  // save dimensions
  jlong* out_num_row = jenv->GetLongArrayElements(jout_num_row, nullptr);
  jlong* out_num_col = jenv->GetLongArrayElements(jout_num_col, nullptr);
  jlong* out_nelem = jenv->GetLongArrayElements(jout_nelem, nullptr);
  out_num_row[0] = static_cast<jlong>(num_row);
  out_num_col[0] = static_cast<jlong>(num_col);
  out_nelem[0] = static_cast<jlong>(num_elem);
  // release arrays
  jenv->ReleaseLongArrayElements(jout_num_row, out_num_row, 0);
  jenv->ReleaseLongArrayElements(jout_num_col, out_num_col, 0);
  jenv->ReleaseLongArrayElements(jout_nelem, out_nelem, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenDMatrixFree
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenDMatrixFree(
    JNIEnv* jenv, jclass jcls, jlong jdmat) {
  return static_cast<int>(TL2cgenDMatrixFree(reinterpret_cast<TL2cgenDMatrixHandle>(jdmat)));
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorLoad
 * Signature: (Ljava/lang/String;I[J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorLoad(
    JNIEnv* jenv, jclass jcls, jstring jlibrary_path, jint jnum_worker_thread, jlongArray jout) {
  char const* library_path = jenv->GetStringUTFChars(jlibrary_path, nullptr);
  TL2cgenPredictorHandle out = nullptr;
  int const ret = TL2cgenPredictorLoad(library_path, static_cast<int>(jnum_worker_thread), &out);
  setHandle(jenv, jout, out);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorPredictBatchWithFloat32Out
 * Signature: (JJZZ[F[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorPredictBatchWithFloat32Out(JNIEnv* jenv,
    jclass jcls, jlong jpredictor, jlong jbatch, jboolean jverbose, jboolean jpred_margin,
    jfloatArray jout_result, jlongArray jout_result_size) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  TL2cgenDMatrixHandle dmat = reinterpret_cast<TL2cgenDMatrixHandle>(jbatch);
  jfloat* out_result = jenv->GetFloatArrayElements(jout_result, nullptr);
  jlong* out_result_size = jenv->GetLongArrayElements(jout_result_size, nullptr);
  size_t out_result_size_tmp = 0;

  TL2cgenPredictorOutputHandle output_vector;
  int ret = TL2cgenPredictorCreateOutputVector(predictor, dmat, &output_vector);
  if (ret != 0) {
    jenv->ReleaseFloatArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  ret = TL2cgenPredictorPredictBatch(predictor, dmat, (jverbose == JNI_TRUE ? 1 : 0),
      (jpred_margin == JNI_TRUE ? 1 : 0), output_vector, &out_result_size_tmp);
  out_result_size[0] = static_cast<jlong>(out_result_size_tmp);
  if (ret != 0) {
    jenv->ReleaseFloatArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  float const* raw_ptr;
  ret = TL2cgenPredictorGetRawPointerFromOutputVector(
      output_vector, reinterpret_cast<void const**>(&raw_ptr));
  if (ret != 0) {
    jenv->ReleaseFloatArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  std::memcpy(out_result, raw_ptr, out_result_size_tmp * sizeof(float));

  jenv->ReleaseFloatArrayElements(jout_result, out_result, 0);
  jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
  ret = TL2cgenPredictorDeleteOutputVector(output_vector);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorPredictBatchWithFloat64Out
 * Signature: (JJZZ[D[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorPredictBatchWithFloat64Out(JNIEnv* jenv,
    jclass jcls, jlong jpredictor, jlong jbatch, jboolean jverbose, jboolean jpred_margin,
    jdoubleArray jout_result, jlongArray jout_result_size) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  TL2cgenDMatrixHandle dmat = reinterpret_cast<TL2cgenDMatrixHandle>(jbatch);
  jdouble* out_result = jenv->GetDoubleArrayElements(jout_result, nullptr);
  jlong* out_result_size = jenv->GetLongArrayElements(jout_result_size, nullptr);
  size_t out_result_size_tmp = 0;

  TL2cgenPredictorOutputHandle output_vector;
  int ret = TL2cgenPredictorCreateOutputVector(predictor, dmat, &output_vector);
  if (ret != 0) {
    jenv->ReleaseDoubleArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  ret = TL2cgenPredictorPredictBatch(predictor, dmat, (jverbose == JNI_TRUE ? 1 : 0),
      (jpred_margin == JNI_TRUE ? 1 : 0), output_vector, &out_result_size_tmp);
  out_result_size[0] = static_cast<jlong>(out_result_size_tmp);
  if (ret != 0) {
    jenv->ReleaseDoubleArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  double const* raw_ptr;
  ret = TL2cgenPredictorGetRawPointerFromOutputVector(
      output_vector, reinterpret_cast<void const**>(&raw_ptr));
  if (ret != 0) {
    jenv->ReleaseDoubleArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  std::memcpy(out_result, raw_ptr, out_result_size_tmp * sizeof(double));

  jenv->ReleaseDoubleArrayElements(jout_result, out_result, 0);
  jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
  ret = TL2cgenPredictorDeleteOutputVector(output_vector);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorPredictBatchWithUInt32Out
 * Signature: (JJZZ[I[J)I
 */
JNIEXPORT jint JNICALL
Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorPredictBatchWithUInt32Out(JNIEnv* jenv,
    jclass jcls, jlong jpredictor, jlong jbatch, jboolean jverbose, jboolean jpred_margin,
    jintArray jout_result, jlongArray jout_result_size) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  TL2cgenDMatrixHandle dmat = reinterpret_cast<TL2cgenDMatrixHandle>(jbatch);
  static_assert(sizeof(jint) == sizeof(uint32_t), "jint wrong size");
  jint* out_result = jenv->GetIntArrayElements(jout_result, nullptr);
  jlong* out_result_size = jenv->GetLongArrayElements(jout_result_size, nullptr);
  size_t out_result_size_tmp = 0;

  TL2cgenPredictorOutputHandle output_vector;
  int ret = TL2cgenPredictorCreateOutputVector(predictor, dmat, &output_vector);
  if (ret != 0) {
    jenv->ReleaseIntArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  ret = TL2cgenPredictorPredictBatch(predictor, dmat, (jverbose == JNI_TRUE ? 1 : 0),
      (jpred_margin == JNI_TRUE ? 1 : 0), output_vector, &out_result_size_tmp);
  out_result_size[0] = static_cast<jlong>(out_result_size_tmp);
  if (ret != 0) {
    jenv->ReleaseIntArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  uint32_t const* raw_ptr;
  ret = TL2cgenPredictorGetRawPointerFromOutputVector(
      output_vector, reinterpret_cast<void const**>(&raw_ptr));
  if (ret != 0) {
    jenv->ReleaseIntArrayElements(jout_result, out_result, 0);
    jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
    return static_cast<jint>(ret);
  }

  std::memcpy(out_result, raw_ptr, out_result_size_tmp * sizeof(uint32_t));

  jenv->ReleaseIntArrayElements(jout_result, out_result, 0);
  jenv->ReleaseLongArrayElements(jout_result_size, out_result_size, 0);
  ret = TL2cgenPredictorDeleteOutputVector(output_vector);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryResultSize
 * Signature: (JJ[J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryResultSize(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jlong jbatch, jlongArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  TL2cgenDMatrixHandle dmat = reinterpret_cast<TL2cgenDMatrixHandle>(jbatch);
  size_t result_size = 0;
  int const ret = TL2cgenPredictorQueryResultSize(predictor, dmat, &result_size);
  // store dimension
  jlong* out = jenv->GetLongArrayElements(jout, nullptr);
  out[0] = static_cast<jlong>(result_size);
  jenv->ReleaseLongArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryNumClass
 * Signature: (J[J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryNumClass(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jlongArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  size_t num_class = 0;
  int const ret = TL2cgenPredictorQueryNumClass(predictor, &num_class);
  // store dimension
  jlong* out = jenv->GetLongArrayElements(jout, nullptr);
  out[0] = static_cast<jlong>(num_class);
  jenv->ReleaseLongArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryNumFeature
 * Signature: (J[J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryNumFeature(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jlongArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  size_t num_feature = 0;
  int const ret = TL2cgenPredictorQueryNumFeature(predictor, &num_feature);
  // store dimension
  jlong* out = jenv->GetLongArrayElements(jout, nullptr);
  out[0] = static_cast<jlong>(num_feature);
  jenv->ReleaseLongArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryPredTransform
 * Signature: (J[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryPredTransform(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jobjectArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  char const* pred_transform = nullptr;
  int const ret = TL2cgenPredictorQueryPredTransform(predictor, &pred_transform);
  // store data
  jstring out = nullptr;
  if (pred_transform != nullptr) {
    out = jenv->NewStringUTF(pred_transform);
  }
  jenv->SetObjectArrayElement(jout, 0, out);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQuerySigmoidAlpha
 * Signature: (J[F)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQuerySigmoidAlpha(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jfloatArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  float alpha = std::numeric_limits<float>::quiet_NaN();
  int const ret = TL2cgenPredictorQuerySigmoidAlpha(predictor, &alpha);
  // store data
  jfloat* out = jenv->GetFloatArrayElements(jout, nullptr);
  out[0] = static_cast<jfloat>(alpha);
  jenv->ReleaseFloatArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryRatioC
 * Signature: (J[F)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryRatioC(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jfloatArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  float ratio_c = std::numeric_limits<float>::quiet_NaN();
  int const ret = TL2cgenPredictorQueryRatioC(predictor, &ratio_c);
  // store data
  jfloat* out = jenv->GetFloatArrayElements(jout, nullptr);
  out[0] = static_cast<jfloat>(ratio_c);
  jenv->ReleaseFloatArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryGlobalBias
 * Signature: (J[F)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryGlobalBias(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jfloatArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  float bias = std::numeric_limits<float>::quiet_NaN();
  int const ret = TL2cgenPredictorQueryGlobalBias(predictor, &bias);
  // store data
  jfloat* out = jenv->GetFloatArrayElements(jout, nullptr);
  out[0] = static_cast<jfloat>(bias);
  jenv->ReleaseFloatArrayElements(jout, out, 0);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryThresholdType
 * Signature: (J[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryThresholdType(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jobjectArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  char const* threshold_type = nullptr;
  int const ret = TL2cgenPredictorQueryThresholdType(predictor, &threshold_type);
  // store data
  jstring out = nullptr;
  if (threshold_type != nullptr) {
    out = jenv->NewStringUTF(threshold_type);
  }
  jenv->SetObjectArrayElement(jout, 0, out);

  return static_cast<jint>(ret);
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorQueryLeafOutputType
 * Signature: (J[Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorQueryLeafOutputType(
    JNIEnv* jenv, jclass jcls, jlong jpredictor, jobjectArray jout) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  char const* leaf_output_type = nullptr;
  jint const ret = (jint)TL2cgenPredictorQueryLeafOutputType(predictor, &leaf_output_type);
  // store data
  jstring out = nullptr;
  if (leaf_output_type != nullptr) {
    out = jenv->NewStringUTF(leaf_output_type);
  }
  jenv->SetObjectArrayElement(jout, 0, out);

  return ret;
}

/*
 * Class:     ml_dmlc_tl2cgen4j_java_TL2cgenJNI
 * Method:    TL2cgenPredictorFree
 * Signature: (J)I
 */
JNIEXPORT jint JNICALL Java_ml_dmlc_tl2cgen4j_java_TL2cgenJNI_TL2cgenPredictorFree(
    JNIEnv* jenv, jclass jcls, jlong jpredictor) {
  TL2cgenPredictorHandle predictor = reinterpret_cast<TL2cgenPredictorHandle>(jpredictor);
  return static_cast<jint>(TL2cgenPredictorFree(predictor));
}
