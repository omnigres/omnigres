// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <utils/builtins.h>
#if PG_MAJORVERSION_NUM >= 16
#include <varatt.h>
#endif

#include <libfyaml.h>

PG_MODULE_MAGIC;

Datum converter(PG_FUNCTION_ARGS, enum fy_emitter_cfg_flags flags) {
  struct fy_parse_cfg parse_cfg = {};
  text *src = PG_GETARG_TEXT_PP(0);
  struct fy_document *fyd =
      fy_document_build_from_string(&parse_cfg, VARDATA_ANY(src), VARSIZE_ANY_EXHDR(src));
  char *string = fy_emit_document_to_string(fyd, flags);
  text *out = cstring_to_text(string);
  free(string);
  fy_document_destroy(fyd);
  PG_RETURN_TEXT_P(out);
}

PG_FUNCTION_INFO_V1(yaml_to_json);
Datum yaml_to_json(PG_FUNCTION_ARGS) { return converter(fcinfo, FYECF_MODE_JSON); }

PG_FUNCTION_INFO_V1(to_yaml);
Datum to_yaml(PG_FUNCTION_ARGS) { return converter(fcinfo, FYECF_MODE_BLOCK | FYECF_MODE_DEJSON); }