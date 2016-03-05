CREATE TYPE sphinx_search_result AS (id int, weight int);

CREATE OR REPLACE FUNCTION sphinx_select(
  /*index*/     varchar,
  /*query*/     varchar,
  /*condition*/ varchar,
  /*order*/     varchar,
  /*offset*/    int,
  /*limit*/     int,
  /*options*/   varchar)
RETURNS SETOF sphinx_search_result
AS 'sphinx', 'pg_sphinx_select'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION sphinx_replace(
  /*index*/     varchar,
  /*id*/        int,
  /*data*/      varchar[])
RETURNS VOID
AS 'sphinx', 'pg_sphinx_replace'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION sphinx_delete(
  /*index*/     varchar,
  /*id*/        int)
RETURNS VOID
AS 'sphinx', 'pg_sphinx_delete'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION sphinx_snippet(
  /*index*/     varchar,
  /*query*/     varchar,
  /*data*/      varchar,
  /*before*/    varchar,
  /*after*/     varchar)
RETURNS VARCHAR
AS 'sphinx', 'pg_sphinx_snippet'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION sphinx_snippet_options(
  /*index*/     varchar,
  /*query*/     varchar,
  /*data*/      varchar,
  /*options*/   varchar[])
RETURNS VARCHAR
AS 'sphinx', 'pg_sphinx_snippet_options'
LANGUAGE C IMMUTABLE;

CREATE TABLE sphinx_config (
  "key"         varchar(32) NOT NULL,
  "value"       varchar(255) NOT NULL,
  PRIMARY KEY ("key")
);

GRANT ALL ON sphinx_config TO PUBLIC;

INSERT INTO sphinx_config ("key", "value") VALUES
  ('host', '127.0.0.1'),
  ('port', '9306'),
  ('username', ''),
  ('password', ''),
  ('prefix', '');

SELECT pg_catalog.pg_extension_config_dump('sphinx_config', '');

