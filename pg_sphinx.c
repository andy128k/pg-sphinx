#include <postgres.h>
#include <fmgr.h>
#include <funcapi.h>
#include <utils/builtins.h>
#include <utils/array.h>
#include <utils/elog.h>
#include <catalog/pg_type.h>
#include <executor/spi.h>

#if PG_VERSION_NUM >= 90300
#include <access/htup_details.h>
#endif

#include "dict.h"
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

PG_FUNCTION_INFO_V1(pg_sphinx_snippet_options);
Datum pg_sphinx_snippet_options(PG_FUNCTION_ARGS);

#define STRLCPY(dst, src, size)                 \
  do {                                          \
    strncpy(dst, src, size);                    \
    dst[size - 1] = '\0';                       \
  } while(0)

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

static int fetch_config(sphinx_config *config)
{
  int result = 0;
  int i;

  default_config(config);

  if (SPI_connect() != SPI_OK_CONNECT)
    goto end;

  if (SPI_execute("SELECT \"key\", \"value\" FROM sphinx_config", true, 0) != SPI_OK_SELECT)
    goto end;

  for (i = 0; i < SPI_processed; ++i)
    {
      char *key, *val;

      key = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 1);
      val = SPI_getvalue(SPI_tuptable->vals[i], SPI_tuptable->tupdesc, 2);

      if (!strcmp(key, "host"))
        STRLCPY(config->host, val, sizeof(config->host));
      else if (!strcmp(key, "port"))
        config->port = atoi(val);
      else if (!strcmp(key, "username"))
        STRLCPY(config->username, val, sizeof(config->username));
      else if (!strcmp(key, "password"))
        STRLCPY(config->password, val, sizeof(config->password));
      else if (!strcmp(key, "prefix"))
        STRLCPY(config->prefix, val, sizeof(config->prefix));

      pfree(key);
      pfree(val);
    }

  result = 1;

end:
  SPI_finish();
  return result;
}

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
          sphinx_config config;

          TO_PSTRING(index,     PG_GETARG_DATUM(0), 0);
          TO_PSTRING(query,     PG_GETARG_DATUM(1), 0);
          TO_PSTRING(condition, PG_GETARG_DATUM(2), PG_ARGISNULL(2));
          TO_PSTRING(order,     PG_GETARG_DATUM(3), PG_ARGISNULL(3));
          offset = PG_ARGISNULL(4) ?  0 : PG_GETARG_UINT32(4);
          limit  = PG_ARGISNULL(5) ? 20 : PG_GETARG_UINT32(5);
          TO_PSTRING(options,   PG_GETARG_DATUM(6), PG_ARGISNULL(6));

          fetch_config(&config);

          funcctx->user_fctx = sphinx_select(&config, &index, &query, &condition, &order, offset, limit, &options, &error);
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

static int array_to_dict(ArrayType *input, Dict *dict)
{
  ArrayIterator iter;
  size_t i;
  Datum value;
  bool isnull;

  // TODO: check element type. ARR_ELEMTYPE(input)

  // one dimensional even length array
  if (ARR_NDIM(input) != 1 || ARR_DIMS(input)[0] % 2 == 1)
    return -1;

  dict->len = ARR_DIMS(input)[0] / 2;
  dict->names = palloc(sizeof(PString) * dict->len);
  dict->values = palloc(sizeof(PString) * dict->len);

#if PG_VERSION_NUM >= 90500
  iter = array_create_iterator(input, 0, NULL);
#else
  iter = array_create_iterator(input, 0);
#endif
  i = 0;
  for (; array_iterate(iter, &value, &isnull); ++i)
    {
      if (i % 2 == 0)
        TO_PSTRING(dict->names[i / 2], value, isnull);
      else
        TO_PSTRING(dict->values[i / 2], value, isnull);
    }
  array_free_iterator(iter);
  return 0;
}

Datum pg_sphinx_replace(PG_FUNCTION_ARGS)
{
  PString index = {0, 0};
  int id;
  ArrayType *input;
  char *error = NULL;
  sphinx_config config;
  Dict data;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_VOID();

  TO_PSTRING(index, PG_GETARG_DATUM(0), 0);
  id = PG_GETARG_UINT32(1);
  input = PG_GETARG_ARRAYTYPE_P(2);

  if (array_to_dict(input, &data))
    PG_RETURN_VOID();

  fetch_config(&config);
  sphinx_replace(&config, &index, id, &data, &error);
  if (error) {
    elog(ERROR, "%s", error);
    free(error);
  }

  pfree(data.names);
  pfree(data.values);

  PG_RETURN_VOID();
}

Datum pg_sphinx_delete(PG_FUNCTION_ARGS)
{
  PString index = {0, 0};
  int id;
  char *error = NULL;
  sphinx_config config;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1))
    PG_RETURN_VOID();

  TO_PSTRING(index, PG_GETARG_DATUM(0), 0);
  id = PG_GETARG_UINT32(1);

  fetch_config(&config);
  sphinx_delete(&config, &index, id, &error);
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
  PString index = {0, 0}, match = {0, 0}, data = {0, 0};

  PString option_names[2] = {PSTRING_CONST("before_match"), PSTRING_CONST("after_match")};
  PString option_values[2] = {{0, 0}, {0, 0}};
  Dict options = {2, option_names, option_values};

  text *result_text = NULL;
  char *error = NULL;
  sphinx_config config;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2))
    PG_RETURN_NULL();

  VARCHAR_TO_PSTRING(index,             PG_GETARG_VARCHAR_P(0), 0);
  VARCHAR_TO_PSTRING(match,             PG_GETARG_VARCHAR_P(1), 0);
  VARCHAR_TO_PSTRING(data,              PG_GETARG_VARCHAR_P(2), 0);
  VARCHAR_TO_PSTRING(options.values[0], PG_GETARG_VARCHAR_P(3), PG_ARGISNULL(3));
  VARCHAR_TO_PSTRING(options.values[1], PG_GETARG_VARCHAR_P(4), PG_ARGISNULL(4));

  fetch_config(&config);
  sphinx_snippet(&config, &index, &match, &data, &options,
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

Datum pg_sphinx_snippet_options(PG_FUNCTION_ARGS)
{
  PString index = {0, 0}, match = {0, 0}, data = {0, 0};
  ArrayType *input;
  Dict options;
  text *result_text = NULL;
  char *error = NULL;
  sphinx_config config;

  if (PG_ARGISNULL(0) || PG_ARGISNULL(1) || PG_ARGISNULL(2) || PG_ARGISNULL(3))
    PG_RETURN_NULL();

  VARCHAR_TO_PSTRING(index, PG_GETARG_VARCHAR_P(0), 0);
  VARCHAR_TO_PSTRING(match, PG_GETARG_VARCHAR_P(1), 0);
  VARCHAR_TO_PSTRING(data,  PG_GETARG_VARCHAR_P(2), 0);
  input = PG_GETARG_ARRAYTYPE_P(3);

  if (array_to_dict(input, &options))
    PG_RETURN_NULL();

  fetch_config(&config);
  sphinx_snippet(&config, &index, &match, &data, &options,
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

