#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <mysql.h>

#include "sphinx.h"
#include "config.h"
#include "stringbuilder.h"
#include "error.h"

static MYSQL *connection = NULL;

static SPH_BOOL ensure_sphinx_is_connected(char **error)
{
  my_bool reconnect;

  if (connection)
    return SPH_TRUE;
  
  connection = mysql_init(NULL);

  reconnect = 1;
  mysql_options(connection, MYSQL_OPT_RECONNECT, &reconnect);

  if (mysql_real_connect(connection, SPHINX_HOST, SPHINX_USER,
                         SPHINX_PASSWORD, NULL, SPHINX_PORT, NULL, 0) == NULL)
    {
      REPORT(error, "Can't connect to sphinx server. ", mysql_error(connection));

      mysql_close(connection);
      connection = NULL;

      return SPH_FALSE;
    }

  return SPH_TRUE;
}

static void string_builder_append_int(StringBuilder *sb, int val)
{
  string_builder_reserve(sb, 40);
  sb->len += snprintf(sb->str + sb->len, 40, "%d", val);
}

static void string_builder_append_quoted(StringBuilder *sb, const PString *str)
{
  string_builder_reserve(sb, 2 * str->len);
  sb->len += mysql_real_escape_string(connection, sb->str + sb->len, str->str, str->len);
}

struct sphinx_context
{
  MYSQL_RES *result;
};

sphinx_context sphinx_select(const PString *index,
                             const PString *match,
                             const PString *condition,
                             const PString *order,
                             int offset,
                             int limit,
                             const PString *options,
                             char **error)
{
  StringBuilder *sb;
  sphinx_context ctx;

  if (!ensure_sphinx_is_connected(error))
    return NULL;

  sb = string_builder_new();
  string_builder_append(sb, "SELECT id, weight() AS weight FROM ");
  string_builder_append_pstr(sb, index);
  string_builder_append(sb, " WHERE MATCH('");
  string_builder_append_quoted(sb, match);
  string_builder_append(sb, "')");
  
  if (PSTR_NOT_EMPTY(condition))
    {
      string_builder_append(sb, " AND ");
      string_builder_append_pstr(sb, condition);
    }

  string_builder_append(sb, " GROUP BY id WITHIN GROUP ORDER BY weight DESC ");

  if (PSTR_NOT_EMPTY(order))
    {
      string_builder_append(sb, " ORDER BY ");
      string_builder_append_pstr(sb, order);
    }
  string_builder_append(sb, " LIMIT ");
  string_builder_append_int(sb, offset);
  string_builder_append(sb, ", ");
  string_builder_append_int(sb, limit);
  if (PSTR_NOT_EMPTY(options))
    {
      string_builder_append(sb, " OPTION ");
      string_builder_append_pstr(sb, options);
    }

  if (mysql_query(connection, sb->str))
    {
      REPORT(error, "Can't execute select query: ", sb->str, "; ", mysql_error(connection));
      string_builder_free(sb);
      return NULL;
    }

  ctx = malloc(sizeof(struct sphinx_context));
  ctx->result = mysql_store_result(connection);

  string_builder_free(sb);
  return ctx;
}

SPH_BOOL sphinx_context_next(sphinx_context ctx,
                             int *id,
                             int *weight)
{
  MYSQL_ROW row;

  if (!ctx)
    return SPH_FALSE;

  row = mysql_fetch_row(ctx->result);
  if (!row)
    return SPH_FALSE;

  if (id)
    *id = row[0] ? atoi(row[0]) : 0;
  if (weight)
    *weight = row[1] ? atoi(row[1]) : 0;

  return SPH_TRUE;
}
    
void sphinx_context_free(sphinx_context ctx)
{
  if (ctx)
    {
      mysql_free_result(ctx->result);
      free(ctx);
    }
}

void sphinx_replace(const PString *index,
                    int id,
                    const PString *columns,
                    const PString *values,
                    size_t count,
                    char **error)
{
  size_t i;
  StringBuilder *sb;

  if (!ensure_sphinx_is_connected(error))
    return;

  sb = string_builder_new();
  string_builder_append(sb, "REPLACE INTO ");
  string_builder_append_pstr(sb, index);
  string_builder_append(sb, " (id");
  for (i = 0; i < count; ++i)
    {
      string_builder_append(sb, ", `");
      string_builder_append_pstr(sb, &columns[i]);
      string_builder_append(sb, "`");
    }
  string_builder_append(sb, ") VALUES (");
  string_builder_append_int(sb, id);
  for (i = 0; i < count; ++i)
    {
      string_builder_append(sb, ", '");
      string_builder_append_quoted(sb, &values[i]);
      string_builder_append(sb, "'");
    }
  string_builder_append(sb, ")");

  if (mysql_query(connection, sb->str))
    REPORT(error, "Can't execute replace query: ", sb->str, "; ", mysql_error(connection));

  string_builder_free(sb);
}

void sphinx_delete(const PString *index,
                   int id,
                   char **error)
{
  StringBuilder *sb;

  if (!ensure_sphinx_is_connected(error))
    return;

  sb = string_builder_new();
  string_builder_append(sb, "DELETE FROM ");
  string_builder_append_pstr(sb, index);
  string_builder_append(sb, " WHERE id = ");
  string_builder_append_int(sb, id);

  if (mysql_query(connection, sb->str))
    REPORT(error, "Can't execute delete query: ", sb->str, "; ", mysql_error(connection));

  string_builder_free(sb);
}

void sphinx_snippet(const PString *index,
                    const PString *match,
                    const PString *data,
                    const PString *before_match,
                    const PString *after_match,
                    return_data_callback callback,
                    void *user_data,
                    char **error)
{
  StringBuilder *sb;
  MYSQL_RES *query_result;
  MYSQL_ROW row;
  unsigned long *lengths;

  if (!callback)
    return;

  if (!ensure_sphinx_is_connected(error))
    return;

  sb = string_builder_new();
  string_builder_append(sb, "CALL SNIPPETS('");
  string_builder_append_quoted(sb, data);
  string_builder_append(sb, "', '");
  string_builder_append_quoted(sb, index);
  string_builder_append(sb, "', '");
  string_builder_append_quoted(sb, match);
  string_builder_append(sb, "', '");
  string_builder_append_quoted(sb, before_match);
  string_builder_append(sb, "' AS before_match, '");
  string_builder_append_quoted(sb, after_match);
  string_builder_append(sb, "' AS after_match)");

  if (mysql_query(connection, sb->str))
    {
      REPORT(error, "Can't execute snippet query: ", sb->str, "; ", mysql_error(connection));
      string_builder_free(sb);
      return;
    }
  
  query_result = mysql_store_result(connection);
  if (!query_result)
    {
      REPORT(error, "Can't store result of snippet query: ", sb->str, "; ", mysql_error(connection));
      string_builder_free(sb);
      return;
    }
  
  row = mysql_fetch_row(query_result);
  if (!row)
    {
      REPORT(error, "Can't fetch result of snippet query: ", sb->str, "; ", mysql_error(connection));
      string_builder_free(sb);
      mysql_free_result(query_result);
      return;
    }

  lengths = mysql_fetch_lengths(query_result);
  callback(row[0], lengths[0], user_data);

  string_builder_free(sb);
  mysql_free_result(query_result);
}

