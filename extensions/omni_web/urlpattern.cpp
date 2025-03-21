#include <ada.h>
#include <ada_c.h>
#include <ada_regex.h>

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wignored-attributes"
#endif
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wregister"
#endif
extern "C" {
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <nodes/execnodes.h>
#include <utils/builtins.h>

#include "urlpattern.h"
}
#ifdef __clang__
#pragma clang diagnostic pop
#endif
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

using regex_provider = ada::pcre2_regex_provider;

auto matches(std::string_view pattern, std::string_view input,
             std::optional<std::string_view> base_url) {
  std::string_view base_url_view(base_url.has_value() ? *base_url : "https://example.com");
  auto base_url_viewp = &base_url_view;

  return ada::parse_url_pattern<regex_provider>(pattern, base_url_viewp, nullptr)
      .and_then([&](auto p) { return p.match(input, base_url_viewp); });
}

extern "C" {
bool matches(char *pattern_data, size_t pattern_size, char *input_data, size_t input_size,
             char *baseurl_data, size_t baseurl_size) {
  try {
    auto input = std::string_view(input_data, input_size);
    auto pattern = std::string_view(pattern_data, pattern_size);
    auto base_url = baseurl_data == nullptr
                        ? std::nullopt
                        : std::optional{std::string_view(baseurl_data, baseurl_size)};
    auto res = matches(pattern, input, base_url);
    if (res.has_value()) {
      return res->has_value();
    } else {
      return false;
    }
  } catch (...) {
    return false;
  }
}

bool match_resultset(ReturnSetInfo *rsinfo, char *pattern_data, size_t pattern_size,
                     char *input_data, size_t input_size, char *baseurl_data, size_t baseurl_size) {
  try {
    auto input = std::string_view(input_data, input_size);
    auto pattern = std::string_view(pattern_data, pattern_size);
    auto base_url = baseurl_data == nullptr
                        ? std::nullopt
                        : std::optional{std::string_view(baseurl_data, baseurl_size)};
    auto res = matches(pattern, input, base_url);
    if (res.has_value()) {
      Datum values[3] = {0, 0, 0};
      bool isnull[3] = {false, false, false};
      auto val = res.value();

#define process(el)                                                                                \
  if (val && val->el.input != "")                                                                  \
    for (auto e : val->el.groups) {                                                                \
      values[0] = PointerGetDatum(cstring_to_text(e.first.c_str()));                               \
      values[1] = PointerGetDatum(cstring_to_text(#el));                                           \
      values[2] = PointerGetDatum(cstring_to_text(e.second->c_str()));                             \
      tuplestore_putvalues(rsinfo->setResult, rsinfo->expectedDesc, values, isnull);               \
    }

      process(protocol);
      process(username);
      process(password);
      process(hostname);
      process(port);
      process(pathname);
      process(search);
      process(hash);

#undef process

      return res->has_value();
    } else {
      return false;
    }
  } catch (...) {
    return false;
  }
}
}
