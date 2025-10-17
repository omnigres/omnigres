#pragma once

#define DOCTEST_CONFIG_IMPLEMENT
#define DOCTEST_CONFIG_NO_SHORT_MACRO_NAMES
#define DOCTEST_CONFIG_NO_MULTITHREADING
#include <doctest.h>

#include <filesystem>
#include <numeric>
#include <ranges>

#include <cppgres.hpp>

#include <apex_omni_shmem.h>

struct test_result {
  std::string name;
  bool passed;
  double seconds;
  std::string failure_reason;
  std::string failed_assertions;
  std::string output;
};

namespace cppgres {
template <> struct type_traits<::test_result> {
  bool is(const type &t) { return t.oid == RECORDOID; }
  constexpr type type_for() { return type{.oid = RECORDOID}; }
};
} // namespace cppgres

class collecting_reporter;

static std::vector<test_result> last_results;

class collecting_reporter : public doctest::IReporter {
  std::ostream &stdout_stream;
  std::vector<test_result> results;

public:
  explicit collecting_reporter(const doctest::ContextOptions &opts) : stdout_stream(*opts.cout) {}

  ~collecting_reporter() override { last_results.swap(results); }

  void test_case_end(const doctest::CurrentTestCaseStats &st) override {
    results.back().passed = st.testCaseSuccess;
    results.back().seconds = st.seconds;
    if (!st.testCaseSuccess) {
      std::vector<std::string> reasons;
      if (st.failure_flags & doctest::TestCaseFailureReason::AssertFailure)
        reasons.push_back("assertion failure");
      if (st.failure_flags & doctest::TestCaseFailureReason::Exception)
        reasons.push_back("threw an exception");
      if (st.failure_flags & doctest::TestCaseFailureReason::Crash)
        reasons.push_back("crash");
      if (st.failure_flags & doctest::TestCaseFailureReason::TooManyFailedAsserts)
        reasons.push_back("too many failed asserts");
      if (st.failure_flags & doctest::TestCaseFailureReason::Timeout)
        reasons.push_back("timeout");
      if (st.failure_flags & doctest::TestCaseFailureReason::ShouldHaveFailedButDidnt)
        reasons.push_back("should have failed but didn't");
      if (st.failure_flags & doctest::TestCaseFailureReason::ShouldHaveFailedAndDid)
        reasons.push_back("should have failed and did");
      if (st.failure_flags & doctest::TestCaseFailureReason::DidntFailExactlyNumTimes)
        reasons.push_back("didn't fail exactly num times");
      if (st.failure_flags & doctest::TestCaseFailureReason::FailedExactlyNumTimes)
        reasons.push_back("failed exactly num times");
      if (st.failure_flags & doctest::TestCaseFailureReason::CouldHaveFailedAndDid)
        reasons.push_back("could have failed and did");

      // TODO: join_with when we reach C++23
      results.back().failure_reason +=
          std::accumulate(std::next(reasons.begin()), reasons.end(), reasons[0],
                          [&](const std::string &a, const std::string &b) { return a + ", " + b; });
    }
    std::ostringstream buffer;
    buffer << stdout_stream.rdbuf();
    results.back().output = buffer.str();
    stdout_stream.clear();
  }

  void test_run_start() override {}
  void test_run_end(const doctest::TestRunStats &) override {}
  void test_case_start(const doctest::TestCaseData &tc) override {
    results.push_back({.name = tc.m_name});
  }
  void test_case_reenter(const doctest::TestCaseData &) override {}
  void test_case_exception(const doctest::TestCaseException &) override {}
  void subcase_start(const doctest::SubcaseSignature &) override {}
  void subcase_end() override {}
  void log_assert(const doctest::AssertData &ad) override {
    if (ad.m_failed) {
      results.back().failed_assertions +=
          cppgres::fmt::format("[{}:{}] {}\n", std::filesystem::path(ad.m_file).filename().string(),
                               ad.m_line, ad.m_decomp.c_str());
    }
  }
  void log_message(const doctest::MessageData &) override {}
  void test_case_skipped(const doctest::TestCaseData &) override {}
  void report_query(const doctest::QueryData &) override {}
};

DOCTEST_REGISTER_REPORTER("collecting", 1, collecting_reporter);

auto run_tests_impl() {
  std::vector<test_result> results;
  doctest::Context ctx;

  ctx.setOption("reporters", "collecting");
  ctx.run();

  results.swap(last_results);

  return results;
}

postgres_function(run_tests, run_tests_impl);
