#ifndef SPHINX_H_INCLUDED
#define SPHINX_H_INCLUDED

#include "pstring.h"

typedef int SPH_BOOL;
#define SPH_TRUE (1)
#define SPH_FALSE (0)

typedef struct {
  char            host[128];
  unsigned short  port;
  char            username[64];
  char            password[64];
  char            prefix[64];
} sphinx_config;

void default_config(sphinx_config *config);

typedef struct sphinx_context *sphinx_context;

sphinx_context sphinx_select(sphinx_config *config,
                             const PString *index,
                             const PString *match,
                             const PString *condition,
                             const PString *order,
                             int offset,
                             int limit,
                             const PString *options,
                             char **error);
SPH_BOOL sphinx_context_next(sphinx_context ctx,
                             /*OUT*/ int *id,
                             /*OUT*/ int *weight);
void sphinx_context_free(sphinx_context ctx);

void sphinx_replace(sphinx_config *config,
                    const PString *index,
                    int id,
                    const PString *columns,
                    const PString *values,
                    size_t count,
                    char **error);

void sphinx_delete(sphinx_config *config,
                   const PString *index,
                   int id,
                   char **error);

typedef void (*return_data_callback)(void *data, size_t size, void *user_data);

void sphinx_snippet(sphinx_config *config,
                    const PString *index,
                    const PString *match,
                    const PString *data,
                    const PString *before_match,
                    const PString *after_match,
                    return_data_callback callback,
                    void *user_data,
                    char **error);

void sphinx_snippet_options(sphinx_config *config,
                            const PString *index,
                            const PString *match,
                            const PString *data,
                            const PString *options,
                            return_data_callback callback,
                            void *user_data,
                            char **error);

#endif

