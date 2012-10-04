#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <utils/builtins.h>

#include "sphinx.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

static void to_pstring(PString *pstr, Datum d, bool isnull)
{
  if (isnull)
    {
      pstr->str = NULL;
      pstr->len = 0;
    }
  else
    {
      pstr->str = VARDATA(d);
      pstr->len = VARSIZE(d) - VARHDRSZ;
    }
}

PG_FUNCTION_INFO_V1(pg_sphinx_select);
Datum pg_sphinx_select(PG_FUNCTION_ARGS)
{
  FuncCallContext *funcctx;

  if (SRF_IS_FIRSTCALL())
    {
      MemoryContext oldcontext;

      funcctx = SRF_FIRSTCALL_INIT();
      oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

      funcctx->tuple_desc = BlessTupleDesc(RelationNameGetTupleDesc("sphinx_search_result"));

      if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
	funcctx->user_fctx = NULL;
      else
	{
	  PString index = {0,}, query = {0,}, condition = {0,}, order = {0,}, options = {0,};
	  int offset, limit;
	  to_pstring(&index,     PG_GETARG_DATUM(0), PG_ARGISNULL(0));
	  to_pstring(&query,     PG_GETARG_DATUM(1), PG_ARGISNULL(1));
	  to_pstring(&condition, PG_GETARG_DATUM(2), PG_ARGISNULL(2));
	  to_pstring(&order,     PG_GETARG_DATUM(3), PG_ARGISNULL(3));
	  offset = PG_ARGISNULL(4) ?  0 : PG_GETARG_UINT32(4);
	  limit  = PG_ARGISNULL(5) ? 20 : PG_GETARG_UINT32(5);
	  to_pstring(&options,   PG_GETARG_DATUM(6), PG_ARGISNULL(6));

	  funcctx->user_fctx = sphinx_select(&index, &query, &condition, &order, offset, limit, &options);
	}
      
      MemoryContextSwitchTo(oldcontext);
    }

  funcctx = SRF_PERCALL_SETUP();
  sphinx_context ctx = funcctx->user_fctx;

  int id, weight;
  if (sphinx_context_next(ctx, &id, &weight))
    {
      Datum result;
      Datum values[2];
      char nulls[2] = {0};

      values[0] = Int32GetDatum(id);
      values[1] = Int32GetDatum(weight);

      result = HeapTupleGetDatum(heap_form_tuple(funcctx->tuple_desc, values, nulls));

      SRF_RETURN_NEXT(funcctx, result);
    }
  else
    {
      sphinx_context_free(ctx);
      SRF_RETURN_DONE(funcctx);
    }
}

