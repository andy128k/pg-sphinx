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

