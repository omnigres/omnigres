// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on
#include <executor/spi.h>
#include <miscadmin.h>
#include <utils/builtins.h>
#include <utils/snapmgr.h>
#if PG_MAJORVERSION_NUM < 15
#include <access/xact.h>
#endif

PG_MODULE_MAGIC;

static int32 retry_attempts = 0;

PG_FUNCTION_INFO_V1(retry);

Datum retry(PG_FUNCTION_ARGS) {
  if (PG_ARGISNULL(0)) {
    ereport(ERROR, errmsg("transaction statements argument is required"));
  }
  if (IsTransactionBlock()) {
    ereport(ERROR, errmsg("can't be used inside of a transaction block"));
  }
  int max_attempts = 10;
  if (!PG_ARGISNULL(1)) {
    max_attempts = PG_GETARG_INT32(1);
  }

  text *stmts = PG_GETARG_TEXT_PP(0);
  char *cstmts = text_to_cstring(stmts);

  bool retry = true;
  retry_attempts = 0;
  while (retry) {
    XactIsoLevel = XACT_SERIALIZABLE;
    SPI_connect_ext(SPI_OPT_NONATOMIC);

    MemoryContext current_mcxt = CurrentMemoryContext;
    PG_TRY();
    {
      SPI_exec(cstmts, 0);
      SPI_finish();
      retry = false;
    }
    PG_CATCH();
    {
      MemoryContextSwitchTo(current_mcxt);
      SPI_finish();
      ErrorData *err = CopyErrorData();
      if (err->sqlerrcode == ERRCODE_T_R_SERIALIZATION_FAILURE) {
        ereport(NOTICE, errmsg("%d", max_attempts));
        if (++retry_attempts > max_attempts) {
          ereport(ERROR, errcode(err->sqlerrcode),
                  errmsg("maximum number of retries (%d) has been attempted", max_attempts));
        }
        PopActiveSnapshot();
        AbortCurrentTransaction();

        SetCurrentStatementStartTimestamp();
        StartTransactionCommand();
        PushActiveSnapshot(GetTransactionSnapshot());

      } else {
        retry_attempts = 0;
        PG_RE_THROW();
      }
      CHECK_FOR_INTERRUPTS();
      FlushErrorState();
    }

    PG_END_TRY();
  }

  PG_RETURN_VOID();
}

PG_FUNCTION_INFO_V1(current_retry_attempt);

Datum current_retry_attempt(PG_FUNCTION_ARGS) { PG_RETURN_INT32(retry_attempts); }