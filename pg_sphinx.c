#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include <utils/elog.h>
#include <catalog/pg_type.h>

#if PG_VERSION_NUM >= 90300
#include <access/htup_details.h>
#endif

#include "sphinx.h"

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

#define TO_PSTRING(pstr, d, isnull)             \
  do {                                          \
    if (isnull)                                 \
      {                                         \
        pstr.str = NULL;                        \
        pstr.len = 0;                           \
      }                                         \
    else                                        \
      {                                         \
        pstr.str = VARDATA(d);                  \
        pstr.len = VARSIZE(d) - VARHDRSZ;       \
      }                                         \
  } while(0)

#define VARCHAR_TO_PSTRING(pstr, d, isnull)     \
  do {                                          \
    if (isnull)                                 \
      {                                         \
        pstr.str = NULL;                        \
        pstr.len = 0;                           \
      }                                         \
    else                                        \
      {                                         \
        pstr.str = VARDATA(d);                  \
        pstr.len = VARSIZE(d) - VARHDRSZ;       \
      }                                         \
  } while(0)

Datum pg_sphinx_select(PG_FUNCTION_ARGS)
{
  FuncCallContext *funcctx;
  sphinx_context ctx;
  int id, weight;
  char *error = NULL;

  if (SRF_IS_FIRSTCALL())
    {
      MemoryContext oldcontext;

      funcctx = SRF_FIRSTCALL_INIT();
      oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

      funcctx->tuple_desc = BlessTupleDesc(RelationNameGetTupleDesc("sphinx_search_result"));

      if (PG_ARGISNULL(0))
        {
          funcctx->user_fctx = NULL;
          elog(ERROR, "%s", "'index' parameter has to be a valid name of search index but NULL was given.");
        }
      else if (PG_ARGISNULL(1))
        {
          funcctx->user_fctx = NULL;
          elog(ERROR, "%s", "'query' parameter has to be a search query but NULL was given.");
        }
      else
        {
          PString index = {0, 0}, query = {0, 0}, condition = {0, 0}, order = {0, 0}, options = {0, 0};
          int offset, limit;
          TO_PSTRING(index,     PG_GETARG_DATUM(0), 0);
          TO_PSTRING(query,     PG_GETARG_DATUM(1), 0);
          TO_PSTRING(condition, PG_GETARG_DATUM(2), PG_ARGISNULL(2));
          TO_PSTRING(order,     PG_GETARG_DATUM(3), PG_ARGISNULL(3));
          offset = PG_ARGISNULL(4) ?  0 : PG_GETARG_UINT32(4);
          limit  = PG_ARGISNULL(5) ? 20 : PG_GETARG_UINT32(5);
          TO_PSTRING(options,   PG_GETARG_DATUM(6), PG_ARGISNULL(6));

          funcctx->user_fctx = sphinx_select(&index, &query, &condition, &order, offset, limit, &options, &error);
        }

      if (error) {
        elog(ERROR, "%s", error);
        free(error);
        error = NULL;
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
  char *error = NULL;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_VOID();

  TO_PSTRING(index, PG_GETARG_DATUM(0), 0);
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
        TO_PSTRING(columns[i / 2], value, isnull);
      else
        TO_PSTRING(values[i / 2], value, isnull);
    }
  array_free_iterator(iter);

  sphinx_replace(&index, id, columns, values, len, &error);
  if (error) {
    elog(ERROR, "%s", error);
    free(error);
  }

  pfree(columns);
  pfree(values);

  PG_RETURN_VOID();
}

Datum pg_sphinx_delete(PG_FUNCTION_ARGS)
{
  PString index = {0, 0};
  int id;
  char *error = NULL;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
    PG_RETURN_VOID();

  TO_PSTRING(index, PG_GETARG_DATUM(0), 0);
  id = PG_GETARG_UINT32(1);

  sphinx_delete(&index, id, &error);
  if (error) {
    elog(ERROR, "%s", error);
    free(error);
  }

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
  char *error = NULL;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_NULL();

  VARCHAR_TO_PSTRING(index,        PG_GETARG_VARCHAR_P(0), 0);
  VARCHAR_TO_PSTRING(match,        PG_GETARG_VARCHAR_P(1), 0);
  VARCHAR_TO_PSTRING(data,         PG_GETARG_VARCHAR_P(2), 0);
  VARCHAR_TO_PSTRING(before_match, PG_GETARG_VARCHAR_P(3), PG_ARGISNULL(3));
  VARCHAR_TO_PSTRING(after_match,  PG_GETARG_VARCHAR_P(4), PG_ARGISNULL(4));

  sphinx_snippet(&index, &match, &data, &before_match, &after_match,
                 return_text, &result_text, &error);

  if (error) {
    elog(ERROR, "%s", error);
    free(error);
  }

  if (result_text)
    PG_RETURN_TEXT_P(result_text);
  else
    PG_RETURN_NULL();
}

