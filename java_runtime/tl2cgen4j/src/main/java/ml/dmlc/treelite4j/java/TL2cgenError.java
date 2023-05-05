package ml.dmlc.tl2cgen4j.java;

/**
 * Custom error class for TL2cgen
 *
 * @author Hyunsu Cho
 */
public class TL2cgenError extends Exception {
  private static final long serialVersionUID = 1L;
  public TL2cgenError(String message) {
    super(message);
  }
}
