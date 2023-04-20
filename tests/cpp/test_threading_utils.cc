/*!
 * Copyright (c) 2023 by Contributors
 * \file test_threading_utils.cc
 * \author Hyunsu Cho
 * \brief C++ tests for threading utilities
 */
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <tl2cgen/detail/threading_utils/parallel_for.h>
#include <tl2cgen/logging.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <random>
#include <vector>

namespace {

class RandomGenerator {
 public:
  RandomGenerator()
      : rng_(std::random_device()()),
        int_dist_(
            std::numeric_limits<std::int64_t>::min(), std::numeric_limits<std::int64_t>::max()),
        real_dist_(0.0, 1.0) {}

  std::int64_t DrawInteger(std::int64_t low, std::int64_t high) {
    TL2CGEN_CHECK_LT(low, high);
    std::int64_t out = int_dist_(rng_);
    std::int64_t rem = out % (high - low);
    std::int64_t ret;
    if (rem < 0) {
      ret = high + rem;
    } else {
      ret = low + rem;
    }
    TL2CGEN_CHECK_GE(ret, low);
    TL2CGEN_CHECK_LT(ret, high);
    return ret;
  }

  double DrawReal(double low, double high) {
    TL2CGEN_CHECK_LT(low, high);
    return real_dist_(rng_) * (high - low) + low;
  }

 private:
  std::mt19937 rng_;
  std::uniform_int_distribution<std::int64_t> int_dist_;
  std::uniform_real_distribution<double> real_dist_;
};

}  // namespace

namespace tl2cgen::detail::threading_utils {
TEST(ThreadingUtils, ParallelFor) {
  /* Test error handling */
  int const max_thread = MaxNumThread();
  auto sched = ParallelSchedule::Guided();

  EXPECT_THROW(threading_utils::ConfigureThreadConfig(max_thread * 3), tl2cgen::Error);

  /* Property-based testing with randomly generated parameters */
  constexpr int kVectorLength = 10000;
  RandomGenerator rng;
  std::vector<double> a(kVectorLength);
  std::vector<double> b(kVectorLength);
  std::generate_n(a.begin(), kVectorLength, [&rng]() { return rng.DrawReal(-1.0, 1.0); });
  std::generate_n(b.begin(), kVectorLength, [&rng]() { return rng.DrawReal(-10.0, 10.0); });

  constexpr int kNumTrial = 200;
  for (int i = 0; i < kNumTrial; ++i) {
    std::vector<double> c(kVectorLength);
    // Fill c with dummy values
    std::generate_n(c.begin(), kVectorLength, [&rng]() { return rng.DrawReal(100.0, 200.0); });

    // Compute c := a + b on range [begin, end)
    std::int64_t begin = rng.DrawInteger(0, kVectorLength);
    auto thread_config = threading_utils::ConfigureThreadConfig(
        static_cast<int>(rng.DrawInteger(1, max_thread + 1)));
    std::int64_t end = rng.DrawInteger(begin, kVectorLength);

    ParallelFor(begin, end, thread_config, sched,
        [&a, &b, &c](std::int64_t k, int) { c[k] = a[k] + b[k]; });

    for (std::int64_t k = begin; k < end; ++k) {
      EXPECT_FLOAT_EQ(c[k], a[k] + b[k]) << ", at index " << k;
    }
  }
}

}  // namespace tl2cgen::detail::threading_utils
