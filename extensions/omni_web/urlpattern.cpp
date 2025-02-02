// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

#include <ada.h>
#include <ada_c.h>
#include <ada_regex.h>

extern "C" {
#include <nodes/execnodes.h>
#include <utils/builtins.h>
}

using regex_provider = ada::pcre2_regex_provider;

auto matches(std::string_view pattern, std::string_view input) {
  std::string new_url;

  // default base URL
  std::string v = "https://example.com";

  // check if input has a base URL
  auto url = ada::parse<ada::url_aggregator>(input);
  if (url && url->has_hostname()) {
    // if it does, replace the default
    v.clear();
    v.append(url->get_protocol());
    v.append("//");
    v.append(url->get_hostname());
  } else {
    // otherwise, compose a new URL
    new_url.append(v);
    new_url.append(input);
    input = std::string_view(new_url);
  }

  // Prepare the final pattern
  auto vv = std::string_view(v);

  auto urlpat = ada::parse_url_pattern<regex_provider>(pattern, &vv, nullptr);
  return urlpat.and_then([&](auto p) { return p.match(input); });
}

extern "C" {
bool matches(char *pattern_data, size_t pattern_size, char *input_data, size_t input_size) {
  auto input = std::string_view(input_data, input_size);
  auto pattern = std::string_view(pattern_data, pattern_size);
  auto res = matches(pattern, input);
  if (res.has_value()) {
    return res->has_value();
  } else {
    return false;
  }
}

bool match_resultset(ReturnSetInfo *rsinfo, char *pattern_data, size_t pattern_size,
                     char *input_data, size_t input_size) {
  auto input = std::string_view(input_data, input_size);
  auto pattern = std::string_view(pattern_data, pattern_size);
  auto res = matches(pattern, input);
  if (res.has_value()) {
    Datum values[3] = {0, 0, 0};
    bool isnull[3] = {false, false, false};
    auto val = res.value();

#define process(el)                                                                                \
  if (val->el.input != "")                                                                         \
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
}
}