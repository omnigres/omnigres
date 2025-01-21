#include <iostream>
#include <string_view>
#include <string>

#include "inja.hpp"

static bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.size() > str.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), str.rbegin());
}

int main(int argc, char **argv) {
    if (argc == 2) {
        char *filename = argv[1];
        inja::Environment env;
        nlohmann::json data;
        std::string_view s(filename);
        if (ends_with(filename, ".sql")) {
            env.set_line_statement("--##");
            env.set_expression("/*{{", "}}*/"); // Expressions
            env.set_comment("/*{#", "#}*/"); // Comments
            env.set_statement("/*{%", "%}*/"); // Statements {% %} for many things, see below
        }
        std::cout << env.render_file(argv[1], data);
    }
}
