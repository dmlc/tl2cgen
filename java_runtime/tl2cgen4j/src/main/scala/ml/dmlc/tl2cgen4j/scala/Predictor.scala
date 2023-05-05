package ml.dmlc.tl2cgen4j.scala

import java.io.IOException

import ml.dmlc.tl2cgen4j.java.{DMatrix, TL2cgenError, Predictor => JPredictor}
import org.nd4j.linalg.api.ndarray.INDArray

import scala.reflect.ClassTag

/**
 * Scala wrapper of ml.dmlc.tl2cgen4j.java.Predictor, keeping same interface as Java Predictor.
 *
 * DEVELOPER WARNING: A Java Predictor must not be shared by more than one Scala Predictor.
 *
 * @param pred the Java Predictor object.
 */
class Predictor private[tl2cgen4j](private[tl2cgen4j] val pred: JPredictor)
    extends Serializable {

  @throws(classOf[TL2cgenError])
  def numFeature: Int = pred.GetNumFeature()

  @throws(classOf[TL2cgenError])
  def numClass: Int = pred.GetNumClass()

  @throws(classOf[TL2cgenError])
  def predTransform: String = pred.GetPredTransform()

  @throws(classOf[TL2cgenError])
  def sigmoidAlpha: Float = pred.GetSigmoidAlpha()

  @throws(classOf[TL2cgenError])
  def ratioC: Float = pred.GetRatioC()

  @throws(classOf[TL2cgenError])
  def globalBias: Float = pred.GetGlobalBias()

  @throws(classOf[TL2cgenError])
  def predictBatch(
      batch: DMatrix,
      predMargin: Boolean = false,
      verbose: Boolean = false): INDArray = {
    pred.predict(batch, verbose, predMargin)
  }

  override def finalize(): Unit = {
    super.finalize()
    dispose()
  }

  def dispose(): Unit = pred.dispose()

}

object Predictor {
  /**
   * @param libPath   Path to the shared library
   * @param numThread Number of workers threads to spawn. Set to -1 to use default,
   *                  i.e., to launch as many threads as CPU cores available on
   *                  the system. You are not allowed to launch more threads than
   *                  CPU cores. Setting ``nthread=1`` indicates that the main
   *                  thread should be exclusively used.
   * @param verbose   Whether to print extra diagnostic messages
   */
  def apply(
      libPath: String,
      numThread: Int = -1,
      verbose: Boolean = true): Predictor = {
    new Predictor(new JPredictor(libPath, numThread, verbose))
  }
}
