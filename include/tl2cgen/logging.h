/*!
 * Copyright (c) 2023 by Contributors
 * \file logging.h
 * \brief logging facility for TL2cgen
 * \author Hyunsu Cho
 */
#ifndef TL2CGEN_LOGGING_H_
#define TL2CGEN_LOGGING_H_

#include <tl2cgen/error.h>
#include <tl2cgen/thread_local.h>

#include <cstdio>
#include <ctime>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>

namespace tl2cgen {

template <typename X, typename Y>
std::unique_ptr<std::string> LogCheckFormat(X const& x, Y const& y) {
  std::ostringstream os;
  os << " (" << x << " vs. " << y << ") ";
  /* CHECK_XX(x, y) requires x and y can be serialized to string. Use CHECK(x OP y) otherwise. */
  return std::make_unique<std::string>(os.str());
}

#if defined(__GNUC__) || defined(__clang__)
#define TL2CGEN_ALWAYS_INLINE inline __attribute__((__always_inline__))
#elif defined(_MSC_VER)
#define TL2CGEN_ALWAYS_INLINE __forceinline
#else
#define TL2CGEN_ALWAYS_INLINE inline
#endif

#define TL2CGEN_DEFINE_CHECK_FUNC(name, op)                                                   \
  template <typename X, typename Y>                                                           \
  TL2CGEN_ALWAYS_INLINE std::unique_ptr<std::string> LogCheck##name(const X& x, const Y& y) { \
    if (x op y)                                                                               \
      return nullptr;                                                                         \
    return LogCheckFormat(x, y);                                                              \
  }                                                                                           \
  TL2CGEN_ALWAYS_INLINE std::unique_ptr<std::string> LogCheck##name(int x, int y) {           \
    return LogCheck##name<int, int>(x, y);                                                    \
  }

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wsign-compare"
TL2CGEN_DEFINE_CHECK_FUNC(_LT, <)
TL2CGEN_DEFINE_CHECK_FUNC(_GT, >)
TL2CGEN_DEFINE_CHECK_FUNC(_LE, <=)
TL2CGEN_DEFINE_CHECK_FUNC(_GE, >=)
TL2CGEN_DEFINE_CHECK_FUNC(_EQ, ==)
TL2CGEN_DEFINE_CHECK_FUNC(_NE, !=)
#pragma GCC diagnostic pop

#define TL2CGEN_CHECK_BINARY_OP(name, op, x, y)                   \
  if (auto __tl2cgen__log__err = ::tl2cgen::LogCheck##name(x, y)) \
  ::tl2cgen::LogMessageFatal(__FILE__, __LINE__).stream()         \
      << "Check failed: " << #x " " #op " " #y << *__tl2cgen__log__err << ": "
#define TL2CGEN_CHECK(x) \
  if (!(x))              \
  ::tl2cgen::LogMessageFatal(__FILE__, __LINE__).stream() << "Check failed: " #x << ": "
#define TL2CGEN_CHECK_LT(x, y) TL2CGEN_CHECK_BINARY_OP(_LT, <, x, y)
#define TL2CGEN_CHECK_GT(x, y) TL2CGEN_CHECK_BINARY_OP(_GT, >, x, y)
#define TL2CGEN_CHECK_LE(x, y) TL2CGEN_CHECK_BINARY_OP(_LE, <=, x, y)
#define TL2CGEN_CHECK_GE(x, y) TL2CGEN_CHECK_BINARY_OP(_GE, >=, x, y)
#define TL2CGEN_CHECK_EQ(x, y) TL2CGEN_CHECK_BINARY_OP(_EQ, ==, x, y)
#define TL2CGEN_CHECK_NE(x, y) TL2CGEN_CHECK_BINARY_OP(_NE, !=, x, y)

#define TL2CGEN_LOG_INFO ::tl2cgen::LogMessage(__FILE__, __LINE__)
#define TL2CGEN_LOG_ERROR TL2CGEN_LOG_INFO
#define TL2CGEN_LOG_WARNING ::tl2cgen::LogMessageWarning(__FILE__, __LINE__)
#define TL2CGEN_LOG_FATAL ::tl2cgen::LogMessageFatal(__FILE__, __LINE__)
#define TL2CGEN_LOG(severity) TL2CGEN_LOG_##severity.stream()

class DateLogger {
 public:
  DateLogger() {
#if defined(_MSC_VER)
    _tzset();
#endif  // defined(_MSC_VER)
  }
  char const* HumanDate() {
#if defined(_MSC_VER)
    _strtime_s(buffer_, sizeof(buffer_));
#else  // defined(_MSC_VER)
    time_t time_value = std::time(nullptr);
    struct tm* pnow;
#if !defined(_WIN32)
    struct tm now;
    pnow = localtime_r(&time_value, &now);
#else  // !defined(_WIN32)
    pnow = std::localtime(&time_value);  // NOLINT(*)
#endif  // !defined(_WIN32)
    std::snprintf(
        buffer_, sizeof(buffer_), "%02d:%02d:%02d", pnow->tm_hour, pnow->tm_min, pnow->tm_sec);
#endif  // defined(_MSC_VER)
    return buffer_;
  }

 private:
  char buffer_[9];
};

class LogMessageFatal {
 public:
  LogMessageFatal(char const* file, int line) {
    log_stream_ << "[" << pretty_date_.HumanDate() << "] " << file << ":" << line << ": ";
  }
  LogMessageFatal(LogMessageFatal const&) = delete;
  void operator=(LogMessageFatal const&) = delete;

  std::ostringstream& stream() {
    return log_stream_;
  }
  ~LogMessageFatal() noexcept(false) {
    throw Error(log_stream_.str());
  }

 private:
  std::ostringstream log_stream_;
  DateLogger pretty_date_;
};

class LogMessage {
 public:
  LogMessage(char const* file, int line) {
    log_stream_ << "[" << DateLogger().HumanDate() << "] " << file << ":" << line << ": ";
  }
  ~LogMessage() {
    Log(log_stream_.str());
  }
  std::ostream& stream() {
    return log_stream_;
  }
  static void Log(std::string const& msg);

 private:
  std::ostringstream log_stream_;
};

class LogMessageWarning {
 public:
  LogMessageWarning(char const* file, int line) {
    log_stream_ << "[" << DateLogger().HumanDate() << "] " << file << ":" << line << ": ";
  }
  ~LogMessageWarning() {
    Log(log_stream_.str());
  }
  std::ostream& stream() {
    return log_stream_;
  }
  static void Log(std::string const& msg);

 private:
  std::ostringstream log_stream_;
};

class LogCallbackRegistry {
 public:
  using Callback = void (*)(char const*);
  LogCallbackRegistry()
      : log_callback_info_([](char const* msg) { std::cerr << msg << std::endl; }),
        log_callback_warn_([](char const* msg) { std::cerr << msg << std::endl; }) {}
  inline void RegisterCallBackLogInfo(Callback log_callback) {
    this->log_callback_info_ = log_callback;
  }
  inline Callback GetCallbackLogInfo() const {
    return log_callback_info_;
  }
  inline void RegisterCallBackLogWarning(Callback log_callback) {
    this->log_callback_warn_ = log_callback;
  }
  inline Callback GetCallbackLogWarning() const {
    return log_callback_warn_;
  }

 private:
  Callback log_callback_info_;
  Callback log_callback_warn_;
};

using LogCallbackRegistryStore = ThreadLocalStore<LogCallbackRegistry>;

}  // namespace tl2cgen

#endif  // TL2CGEN_LOGGING_H_
