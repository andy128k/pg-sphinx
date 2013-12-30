#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include <catalog/pg_type.h>

#if PG_VERSION_NUM >= 90300
#include <access/htup_details.h>
#endif

#include "sphinx.h"
#include "log.h"

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

PG_FUNCTION_INFO_V1(pg_sphinx_select);
Datum pg_sphinx_select(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pg_sphinx_replace);
Datum pg_sphinx_replace(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pg_sphinx_delete);
Datum pg_sphinx_delete(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(pg_sphinx_snippet);
Datum pg_sphinx_snippet(PG_FUNCTION_ARGS);

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

static void varchar_to_pstring(PString *pstr, VarChar *d, bool isnull)
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

Datum pg_sphinx_select(PG_FUNCTION_ARGS)
{
  FuncCallContext *funcctx;
  sphinx_context ctx;
  int id, weight;

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
          PString index = {0, 0}, query = {0, 0}, condition = {0, 0}, order = {0, 0}, options = {0, 0};
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
  ctx = funcctx->user_fctx;

  if (sphinx_context_next(ctx, &id, &weight))
    {
      Datum result;
      Datum values[2];
      char nulls[2] = {0, 0};

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

Datum pg_sphinx_replace(PG_FUNCTION_ARGS)
{
  PString index = {0, 0};
  int id;
  ArrayType *input;
  size_t len;
  PString *columns, *values;
  ArrayIterator iter;
  Datum value;
  bool isnull;
  size_t i;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_VOID();

  to_pstring(&index, PG_GETARG_DATUM(0), PG_ARGISNULL(0));
  id = PG_GETARG_UINT32(1);
  input = PG_GETARG_ARRAYTYPE_P(2);

  // TODO: check element type. ARR_ELEMTYPE(input)

  // one dimensional even length array
  if (ARR_NDIM(input) != 1 || ARR_DIMS(input)[0] % 2 == 1)
    PG_RETURN_VOID();

  len = ARR_DIMS(input)[0] / 2;
  columns = palloc(sizeof(PString) * len);
  values  = palloc(sizeof(PString) * len);

  iter = array_create_iterator(input, 0);
  i = 0;
  for (; array_iterate(iter, &value, &isnull); ++i)
    {
      if (i % 2 == 0)
        to_pstring(&columns[i / 2], value, isnull);
      else
        to_pstring(&values[i / 2], value, isnull);
    }
  array_free_iterator(iter);

  sphinx_replace(&index, id, columns, values, len);

  pfree(columns);
  pfree(values);

  PG_RETURN_VOID();
}

Datum pg_sphinx_delete(PG_FUNCTION_ARGS)
{
  PString index = {0, 0};
  int id;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
    PG_RETURN_VOID();

  to_pstring(&index, PG_GETARG_DATUM(0), PG_ARGISNULL(0));
  id = PG_GETARG_UINT32(1);

  sphinx_delete(&index, id);

  PG_RETURN_VOID();
}

static void return_text(void *data, size_t size, void *user_data)
{
  if (user_data)
    {
      text *r = palloc(VARHDRSZ + size);
      SET_VARSIZE(r, VARHDRSZ + size);
      memcpy((void *)VARDATA(r), data, size);

      *(text **)user_data = r;
    }
}

Datum pg_sphinx_snippet(PG_FUNCTION_ARGS)
{
  PString index = {0, 0}, match = {0, 0}, data = {0, 0}, before_match = {0, 0}, after_match = {0, 0};
  text *result_text = NULL;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_NULL();

  varchar_to_pstring(&index,        PG_GETARG_VARCHAR_P(0), PG_ARGISNULL(0));
  varchar_to_pstring(&match,        PG_GETARG_VARCHAR_P(1), PG_ARGISNULL(1));
  varchar_to_pstring(&data,         PG_GETARG_VARCHAR_P(2), PG_ARGISNULL(2));
  varchar_to_pstring(&before_match, PG_GETARG_VARCHAR_P(3), PG_ARGISNULL(3));
  varchar_to_pstring(&after_match,  PG_GETARG_VARCHAR_P(4), PG_ARGISNULL(4));

  sphinx_snippet(&index, &match, &data, &before_match, &after_match,
                 return_text, &result_text);

  PG_RETURN_TEXT_P(result_text);
}

