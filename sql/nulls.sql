SELECT * FROM sphinx_select(NULL, 'database', 'user_id = 12', 'weight DESC, id DESC', 0, 100, NULL);

SELECT * FROM sphinx_select('test_index', NULL, 'user_id = 12', 'weight DESC, id DESC', 0, 100, NULL);

SELECT * FROM sphinx_select('test_index', 'database', NULL, 'weight DESC, id DESC', 0, 100, NULL);

SELECT * FROM sphinx_select('test_index', 'database', 'user_id = 12', NULL, 0, 100, NULL);

SELECT * FROM sphinx_select('test_index', 'database', 'user_id = 12', 'weight DESC, id DESC', NULL, 100, NULL);

SELECT * FROM sphinx_select('test_index', 'database', 'user_id = 12', 'weight DESC, id DESC', 0, NULL, NULL);


SELECT sphinx_snippet(NULL, 'database', 'Text about databases', '<mark>', '</mark>');

SELECT sphinx_snippet('test_index', NULL, 'Text about databases', '<mark>', '</mark>');

SELECT sphinx_snippet('test_index', 'database', NULL, '<mark>', '</mark>');

SELECT sphinx_snippet('test_index', 'database', 'Text about databases', NULL, '</mark>');

SELECT sphinx_snippet('test_index', 'database', 'Text about databases', '<mark>', NULL);


SELECT sphinx_snippet_options(NULL, 'database', 'Text about databases',
  ARRAY['before_match', '<mark>',
        'after_match', '</mark>',
        'limit', '100',
        'allow_empty', 'y']);

SELECT sphinx_snippet_options('test_index', NULL, 'Text about databases',
  ARRAY['before_match', '<mark>',
        'after_match', '</mark>',
        'limit', '100',
        'allow_empty', 'y']);


SELECT sphinx_snippet_options('test_index', 'database', NULL,
  ARRAY['before_match', '<mark>',
        'after_match', '</mark>',
        'limit', '100',
        'allow_empty', 'y']);

SELECT sphinx_snippet_options('test_index', 'database', 'Text about databases', NULL);

SELECT sphinx_snippet_options('test_index', 'database', 'Text about databases',
  ARRAY[]::text[]);

SELECT sphinx_snippet_options('test_index', 'database', 'Text about databases',
  ARRAY['before_match']);

