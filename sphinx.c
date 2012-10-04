#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <mysql.h>

#include "sphinx.h"
#include "config.h"
#include "stringbuilder.h"

static MYSQL *connection;

static SPH_BOOL ensure_sphinx_is_connected(void)
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
      mysql_close(connection);
      connection = NULL;
      return SPH_FALSE;
    }
  
  return SPH_TRUE;
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
			     const PString *options)
{
  char itoa_buffer[40];
  StringBuilder *sb;

  ensure_sphinx_is_connected();

  sb = string_builder_new();
  string_builder_append(sb, "SELECT @id FROM ");
  string_builder_append_pstr(sb, index);
  string_builder_append(sb, " WHERE MATCH('");
  
  string_builder_reserve(sb, 2 * match->len);
  sb->len += mysql_real_escape_string(connection, sb->str + sb->len, match->str, match->len);

  string_builder_append(sb, "')");
  
  if (PSTR_NOT_EMPTY(condition))
    {
      string_builder_append(sb, " AND ");
      string_builder_append_pstr(sb, condition);
    }
  
  string_builder_append(sb, " GROUP BY @id WITHIN GROUP ORDER BY @weight DESC ");

  if (PSTR_NOT_EMPTY(order))
    {
      string_builder_append(sb, " ORDER BY ");
      string_builder_append_pstr(sb, order);
    }
  string_builder_append(sb, " LIMIT ");
  snprintf(itoa_buffer, sizeof(itoa_buffer), "%d", offset);
  string_builder_append(sb, itoa_buffer);
  string_builder_append(sb, ", ");
  snprintf(itoa_buffer, sizeof(itoa_buffer), "%d", limit);
  string_builder_append(sb, itoa_buffer);
  if (PSTR_NOT_EMPTY(options))
    {
      string_builder_append(sb, " OPTION ");
      string_builder_append_pstr(sb, options);
    }

  if (mysql_query(connection, sb->str))
    return NULL;

  sphinx_context ctx = malloc(sizeof(struct sphinx_context));
  ctx->result = mysql_store_result(connection);

  string_builder_free(sb);
  return ctx;
}

SPH_BOOL sphinx_context_next(sphinx_context ctx,
			     int *id,
			     int *weight)
{
  if (!ctx)
    return SPH_FALSE;

  MYSQL_ROW row;
  
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

