#include "ada_regex.h"

std::optional<ada::pcre2_regex_provider::regex_type> ada::pcre2_regex_provider::create_instance(std::string_view pattern,
                                                 bool ignore_case) {
  PCRE2_SIZE erroffset;
  pcre2_code *pc;
  int err;

  pc = pcre2_compile((PCRE2_SPTR)pattern.data(), pattern.length(),
                      PCRE2_UTF | PCRE2_UCP | (ignore_case ? PCRE2_CASELESS : 0),
                      &err, &erroffset,
                      nullptr);

  if (!pc) {
    return std::nullopt;
  }

  return pc;
}

std::optional<std::vector<std::optional<std::string>>> ada::pcre2_regex_provider::regex_search(
    std::string_view input, const ada::pcre2_regex_provider::regex_type& pattern) {

  if (pattern == nullptr) {
    return std::nullopt;
  }

  pcre2_match_data* match_data = pcre2_match_data_create_from_pattern(pattern, nullptr);
  if (!match_data) {
    return std::nullopt;
  }

  int rc = pcre2_match(
      pattern,
      (PCRE2_SPTR)input.data(),
      input.length(),
      0,
      0,
      match_data,
      nullptr
  );

  if (rc == PCRE2_ERROR_NOMATCH || rc < 0) {
    pcre2_match_data_free(match_data);
    return std::nullopt;
  }

  // Retrieve the vector of offsets from the match data.
  PCRE2_SIZE* ovector = pcre2_get_ovector_pointer(match_data);
  std::vector<std::optional<std::string>> results;
  results.reserve(static_cast<size_t>(rc));

  // Iterate over each captured group (group 0 is the full match).
  for (int i = 1; i < rc; i++) {
    PCRE2_SIZE start = ovector[2 * i];
    PCRE2_SIZE end = ovector[2 * i + 1];

    // If the group did not participate in the match, its offsets will be PCRE2_UNSET.
    if (start == PCRE2_UNSET || end == PCRE2_UNSET) {
      results.emplace_back(std::nullopt);
    } else {
      // Construct the matched substring.
      results.emplace_back(std::string(input.substr(start, end - start)));
    }
  }

  pcre2_match_data_free(match_data);
  return results;
}

bool ada::pcre2_regex_provider::regex_match(std::string_view input, const ada::pcre2_regex_provider::regex_type& pattern) {
  if (pattern == nullptr) {
    return false;
  }

  pcre2_match_data *match_data = pcre2_match_data_create_from_pattern(pattern, nullptr);
  int rc = pcre2_match(pattern, (PCRE2_SPTR)input.data(), input.length(), 0, 0, match_data, nullptr);
  bool result = rc >= 0;
  pcre2_match_data_free(match_data);
  return result;
 }