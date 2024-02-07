/*!
 * Copyright (c) by 2023-2024 Contributors
 * \file format_util.h
 * \brief Formatting utilities
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_DETAIL_COMPILER_CODEGEN_FORMAT_UTIL_H_
#define TL2CGEN_DETAIL_COMPILER_CODEGEN_FORMAT_UTIL_H_

#include <fmt/format.h>

#include <iomanip>
#include <limits>
#include <sstream>
#include <string>

namespace tl2cgen::compiler::detail::codegen {

/*!
 * \brief Apply indentation to a multi-line string by inserting spaces at the beginning of each line
 * \param str Multi-line string
 * \param indent Indent level to be applied (in number of spaces)
 * \return Indented string
 */
inline std::string IndentMultiLineString(std::string const& str, int indent) {
  std::istringstream iss{str};
  std::ostringstream oss;
  bool first_line = true;
  for (std::string line; std::getline(iss, line, '\n');) {
    if (first_line) {
      first_line = false;
    } else {
      oss << "\n";
    }
    oss << std::string(indent, ' ') << line;
  }
  return oss.str();
}

/*!
 * \brief obtain a string representation of floating-point value, expressed
 * in high precision
 * \param value a value of primitive type
 * \return string representation
 */
template <typename T>
inline std::string ToStringHighPrecision(T value) {
  return fmt::format("{:.{}g}", value, std::numeric_limits<T>::max_digits10 + 2);
}

/*! \brief Format array as text, wrapped to a given maximum text width. Uses high precision to
 *         render floating-point values. */
class ArrayFormatter {
 public:
  /*!
   * \brief Constructor
   * \param text_width maximum text width
   * \param indent indentation level
   * \param delimiter delimiter between elements
   */
  ArrayFormatter(int text_width, int indent, char delimiter = ',')
      : oss_(),
        text_width_(text_width),
        indent_(indent),
        delimiter_(delimiter),
        default_precision_(static_cast<int>(oss_.precision())),
        line_length_(indent),
        is_empty_(true) {}

  /*!
   * \brief add an entry (will use high precision for floating-point values)
   * \param e entry to be added
   */
  template <typename T>
  inline ArrayFormatter& operator<<(T const& e) {
    if (is_empty_) {
      is_empty_ = false;
      oss_ << std::string(indent_, ' ');
    }
    std::ostringstream tmp;
    tmp << std::setprecision(GetPrecision<T>()) << e << delimiter_ << " ";
    std::string const token = tmp.str();  // token to be added to wrapped text
    if (line_length_ + token.length() <= text_width_) {
      oss_ << token;
      line_length_ += token.length();
    } else {
      oss_ << "\n" << std::string(indent_, ' ') << token;
      line_length_ = token.length() + indent_;
    }
    return *this;
  }

  /*!
   * \brief Obtain formatted text containing the rendered array
   * \return String representing the rendered array
   */
  inline std::string str() {
    return oss_.str();
  }

 private:
  std::ostringstream oss_;  // string stream to store wrapped text
  int const text_width_;  // maximum length of each line
  int const indent_;  // indent level, to indent each line
  char const delimiter_;  // delimiter (defaults to comma)
  int const default_precision_;  // default precision used by string stream
  int line_length_;  // width of current line
  bool is_empty_;  // true if no entry has been added yet

  template <typename T>
  inline int GetPrecision() {
    return default_precision_;
  }
};

template <>
inline int ArrayFormatter::GetPrecision<float>() {
  return std::numeric_limits<float>::digits10 + 2;
}
template <>
inline int ArrayFormatter::GetPrecision<double>() {
  return std::numeric_limits<double>::digits10 + 2;
}

}  // namespace tl2cgen::compiler::detail::codegen

#endif  // TL2CGEN_DETAIL_COMPILER_CODEGEN_FORMAT_UTIL_H_
