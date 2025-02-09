// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <ada.h>
#include <ada_c.h>
#include <ada_regex.h>

extern "C" {
#include <nodes/execnodes.h>

#include "urlpattern.h"
}

template <typename T> static void hash_combine(std::size_t &seed, const std::optional<T> &value) {
  if (value.has_value()) {
    std::size_t h = std::hash<T>()(*value);
    seed ^= h + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  } else {
    seed ^= 0 + 0x9e3779b97f4a7c15ULL + (seed << 6) + (seed >> 2);
  }
}

namespace std {

template <> struct hash<ada::url_pattern_init> {
  size_t operator()(const ada::url_pattern_init &init) const {
    std::size_t seed = 0;

    hash_combine(seed, init.protocol);
    hash_combine(seed, init.username);
    hash_combine(seed, init.password);
    hash_combine(seed, init.hostname);
    hash_combine(seed, init.port);
    hash_combine(seed, init.pathname);
    hash_combine(seed, init.search);
    hash_combine(seed, init.hash);
    hash_combine(seed, init.base_url);

    return seed;
  }
};
} // namespace std

using regex_provider = ada::pcre2_regex_provider;

template <ada::url_pattern_regex::regex_concept regex_provider>
static ada::result<std::optional<ada::url_pattern_result>> matches(ada::url_pattern_init pattern,
                                                                   std::string_view input) {
  static std::unordered_map<ada::url_pattern_init, ada::url_pattern<regex_provider>> pattern_cache;
  auto it = pattern_cache.find(pattern);
  if (it == pattern_cache.end()) {
    auto res = ada::parse_url_pattern<regex_provider>(pattern, nullptr, nullptr);
    if (!res) {
      return std::nullopt;
    }
    pattern_cache[pattern] = *res;
    it = pattern_cache.find(pattern);
  }
  return it->second.match(input, nullptr);
}

extern "C" {

bool match_urlpattern(omni_httpd_urlpattern_t *pat, char *input_data, size_t input_size) {
  try {
    if (pat == nullptr) {
      return false;
    }
    auto input = std::string_view(input_data, input_size);
    auto pattern = ada::url_pattern_init{
#define assign_text(el)                                                                            \
  .el = pat->el == nullptr ? std::nullopt                                                          \
                           : std::optional(std::string(std::string_view(pat->el, pat->el##_len)))
        assign_text(protocol),
        assign_text(username),
        assign_text(password),
        assign_text(hostname),
        .port = pat->port == 0 ? std::nullopt : std::optional(std::to_string(pat->port)),
        assign_text(pathname),
        assign_text(search),
        assign_text(hash),
#undef assign_text
    };
    auto res = matches<regex_provider>(pattern, input);
    return res.has_value() && res->has_value();
  } catch (...) {
    return false;
  }
}
}