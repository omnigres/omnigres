#include "ada.h"

extern "C" {
#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>
}

namespace ada {

class pcre2_code_t {
public:
  pcre2_code_t() noexcept : code_(nullptr) {};

  pcre2_code_t(pcre2_code* code) noexcept : code_(code) {}
  ~pcre2_code_t() noexcept { pcre2_code_free(code_); }

  operator pcre2_code*() const noexcept { return code_; }

  // Copy code on construction and assignment
  pcre2_code_t(const pcre2_code_t& other) : code_(pcre2_code_copy(other.code_)) { }
  pcre2_code_t& operator=(const pcre2_code_t& other)  {
    code_ = pcre2_code_copy(other.code_);
    return *this;
  }

  // Enable move semantics.
  pcre2_code_t(pcre2_code_t&& other) noexcept : code_(other.code_) {
    other.code_ = nullptr;
  }

private:
    pcre2_code* code_;
};

class pcre2_regex_provider {
 public:
  pcre2_regex_provider() = default;
  using regex_type = pcre2_code_t;
  static std::optional<regex_type> create_instance(std::string_view pattern,
                                                   bool ignore_case);
  static std::optional<std::vector<std::optional<std::string>>> regex_search(
      std::string_view input, const regex_type& pattern);
  static bool regex_match(std::string_view input, const regex_type& pattern);
};
}