extern "C" {
// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <miscadmin.h>
#include <nodes/execnodes.h>
#include <utils/builtins.h>

PG_MODULE_MAGIC;
}

#include <iostream>
#include <pugixml.hpp>
#include <sstream>

static Datum xpath_impl(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("document can't be null"));
  }

  if (PG_ARGISNULL(1)) {
    ereport(ERROR, errmsg("query can't be null"));
  }

  text *doc = PG_GETARG_TEXT_PP(0);
  text *query = PG_GETARG_TEXT_PP(1);

  // Prepare to return the set
  ReturnSetInfo *rsinfo = (ReturnSetInfo *)fcinfo->resultinfo;
  rsinfo->returnMode = SFRM_Materialize;

  MemoryContext per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
  MemoryContext oldcontext = MemoryContextSwitchTo(per_query_ctx);

  pugi::xml_document xml;
  pugi::xml_parse_result result = xml.load_string(text_to_cstring(doc));
  if (!result) {
    ereport(ERROR, errmsg("XML parsing error"), errdetail("%s", result.description()));
  }

  pugi::xpath_query xpath_query(text_to_cstring(query));
  if (!xpath_query.result()) {
    ereport(ERROR, errmsg("XPath query error"),
            errdetail("%s", xpath_query.result().description()));
  }
  pugi::xpath_node_set nodeset = xml.select_nodes(xpath_query);

  Tuplestorestate *tupstore = tuplestore_begin_heap(false, false, work_mem);
  rsinfo->setResult = tupstore;

  for (pugi::xpath_node node : nodeset) {
    pugi::xml_node actual_node = node.node();
    text *data = NULL;
    switch (actual_node.type()) {
    case pugi::node_element:
    case pugi::node_document: {
      std::ostringstream oss;
      actual_node.print(oss, "", pugi::format_raw);
      data = cstring_to_text(oss.str().c_str());
      break;
    }
    case pugi::node_cdata:
    case pugi::node_pcdata:
      data = cstring_to_text(actual_node.value());
    default:
      break;
    }
    Datum values[2] = {PointerGetDatum(cstring_to_text(actual_node.path().c_str())),
                       PointerGetDatum(data)};
    bool isnull[2] = {false, data == NULL};
    tuplestore_putvalues(tupstore, rsinfo->expectedDesc, values, isnull);
  }

  tuplestore_donestoring(tupstore);

  MemoryContextSwitchTo(oldcontext);
  PG_RETURN_NULL();
}

static void init() { pugi::set_memory_management_functions(palloc, pfree); }

extern "C" {

void _PG_init() { init(); }

PG_FUNCTION_INFO_V1(xpath);
Datum xpath(PG_FUNCTION_ARGS) { return xpath_impl(fcinfo); }
}
