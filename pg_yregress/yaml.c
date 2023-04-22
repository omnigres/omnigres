#include <libfyaml.h>

bool fy_node_is_boolean(struct fy_node *node) {
  if (!fy_node_is_scalar(node)) {
    return false;
  }
  size_t sz;
  const char *val = fy_node_get_scalar(node, &sz);
#define match(pat)                                                                                 \
  if (strncmp(val, pat, sz) == 0) {                                                                \
    return true;                                                                                   \
  }
  // clang-format off
  match("y") else match("Y") else match("yes") else match("Yes") else match("YES") else
  match("n") else match("N") else match("no") else match("No") else match("NO") else
  match("true") else match("True") else match("TRUE") else
  match("false") else match("False") else match("FALSE") else
  match("on") else match("On") else match("ON") else
  match("off") else match("Off") else match("OFF") else
  return false;
  // clang-format on
#undef match
}

bool fy_node_get_boolean(struct fy_node *node) {
  size_t sz;
  const char *val = fy_node_get_scalar(node, &sz);
#define match(pat) strncmp(val, pat, sz) == 0
  return (match("y") || match("Y") || match("yes") || match("Yes") || match("YES") ||
          match("true") || match("True") || match("TRUE") || match("on") || match("On") ||
          match("ON"));
#undef match
}