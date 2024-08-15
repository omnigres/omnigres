// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <access/xact.h>
#include <executor/spi.h>
#include <utils/builtins.h>
#include <utils/snapmgr.h>

PG_MODULE_MAGIC;

PG_FUNCTION_INFO_V1(spi_exec);

Datum spi_exec(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("statements argument is required"));
  }
  bool atomic = true;
  if (!PG_ARGISNULL(1)) {
    atomic = PG_GETARG_BOOL(1);
  }

  text *stmts = PG_GETARG_TEXT_PP(0);
  char *cstmts = text_to_cstring(stmts);

  SPI_connect_ext(atomic ? 0 : SPI_OPT_NONATOMIC);

  if (!atomic) {
    SetCurrentStatementStartTimestamp();
    StartTransactionCommand();
    PushActiveSnapshot(GetTransactionSnapshot());
  }

  int rc = SPI_exec(cstmts, 0);
  if (rc < 0) {
    ereport(ERROR, errmsg("SPI error: %s", SPI_result_code_string(rc)));
  }
  SPI_finish();

  PopActiveSnapshot();
  CommitTransactionCommand();

  PG_RETURN_VOID();
}