CREATE TYPE sphinx_search_result AS (id int, weight int);

CREATE OR REPLACE FUNCTION sphinx_select(/*index*/     varchar,
                                         /*query*/     varchar,
                                         /*condition*/ varchar,
                                         /*order*/     varchar,
                                         /*offset*/    int,
                                         /*limit*/     int,
                                         /*options*/   varchar)
RETURNS SETOF sphinx_search_result
AS 'pgsphinx.so', 'pg_sphinx_select'
LANGUAGE C IMMUTABLE;

