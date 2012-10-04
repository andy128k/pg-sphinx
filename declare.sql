CREATE TYPE sphinx_search_result AS (id int, weight int);

CREATE OR REPLACE FUNCTION sphinx_select(/*index*/     varchar,
                                         /*query*/     varchar,
                                         /*condition*/ varchar,
                                         /*order*/     varchar,
                                         /*offset*/    int,
                                         /*limit*/     int,
                                         /*options*/   varchar)
RETURNS SETOF sphinx_search_result
AS '/home/andy/projects/pg_sphinx/.libs/libpgsphinx.so', 'pg_sphinx_select'
LANGUAGE C IMMUTABLE;

