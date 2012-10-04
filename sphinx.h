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
			     const PString *options);
SPH_BOOL sphinx_context_next(sphinx_context ctx,
			     /*OUT*/ int *id,
			     /*OUT*/ int *weight);
void sphinx_context_free(sphinx_context ctx);

#endif

