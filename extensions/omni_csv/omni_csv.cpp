#include <cppgres.hpp>

extern "C" {
PG_MODULE_MAGIC;
}

#include <csv.hpp>
#include <ranges>

constexpr auto default_format() {
  csv::CSVFormat format;
  format.delimiter({'|', ',', '\t'});
  return format;
}

std::vector<std::string> csv_info_function(std::string_view csv) {
  csv::CSVFormat format = default_format();
  auto doc = csv::parse(csv, format);
  return doc.get_col_names();
}

postgres_function(csv_info, csv_info_function);

auto parse_function(std::string_view csv) {
  csv::CSVFormat format = default_format();
  auto doc = csv::parse(csv, format);

  std::vector<cppgres::record> result;

  auto size = doc.get_col_names().size();
  cppgres::tuple_descriptor td(size);
  for (auto i = 0; i < size; i++) {
    td.set_type(i, cppgres::type{.oid = TEXTOID});
  }

  for (auto &row : doc) {
    std::vector<std::string> values = row;
    result.emplace_back(td, values.begin(), values.end());
  }

  return result;
}

postgres_function(parse, parse_function);

struct csv_aggregate {
  bool header_written = false;
  std::stringstream buffer;
  csv::CSVWriter<std::stringstream> writer;

  csv_aggregate() : writer(buffer) {}

  void update(cppgres::record &record) {
    auto td = record.get_tuple_descriptor();

    // Prepare the header
    if (!header_written) {
      std::vector<std::string> header;
      for (auto i = 0; i < td.attributes(); i++) {
        header.emplace_back(td.get_name(i));
      }
      writer << header;
      header_written = true;
    }

    // Figure out how to output the types
    std::vector<regproc> output;
    for (auto i = 0; i < td.attributes(); i++) {
      cppgres::syscache<Form_pg_type, cppgres::oid> cache(td.get_type(i).oid);
      output.push_back((*cache).typoutput);
    }

    auto it =
        std::views::iota(0, record.attributes()) | std::views::transform([&](int i) {
          return cppgres::ffi_guard{::OidOutputFunctionCall}(output[i], record.get_attribute(i));
        });

    writer << it;
  }

  std::string finalize() const { return buffer.str(); }
};

declare_aggregate(csv, csv_aggregate, cppgres::record);
