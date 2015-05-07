#ifndef SPHINX_H_INCLUDED
#define SPHINX_H_INCLUDED

#include "pstring.h"

typedef int SPH_BOOL;
#define SPH_TRUE (1)
#define SPH_FALSE (0)

typedef struct sphinx_context *sphinx_context;

sphinx_context sphinx_select(const PString *index,
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

void sphinx_replace(const PString *index,
                    int id,
                    const PString *columns,
                    const PString *values,
                    size_t count,
                    char **error);

void sphinx_delete(const PString *index,
                   int id,
                   char **error);

typedef void (*return_data_callback)(void *data, size_t size, void *user_data);

void sphinx_snippet(const PString *index,
                    const PString *match,
                    const PString *data,
                    const PString *before_match,
                    const PString *after_match,
                    return_data_callback callback,
                    void *user_data,
                    char **error);

#endif

