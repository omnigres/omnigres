#include "inja.hpp"

extern "C" {
char *render_yaml_file(char *filename) {
  inja::Environment env;
  nlohmann::json data = {};
  return strdup(env.render_file(filename, data).c_str());
}
}
