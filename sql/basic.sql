CREATE EXTENSION sphinx;

SELECT sphinx_replace('test_index', 5, ARRAY['title', 'Title', 'content', 'Text about databases', 'user_id', 12]::text[]);

SELECT sphinx_replace('test_index', 6, ARRAY['title', 'Caption', 'content', 'Content of database manual', 'user_id', 12]::text[]);

SELECT * FROM sphinx_select('test_index', 'database', 'user_id = 12', 'weight DESC, id DESC', 0, 100, NULL);

SELECT sphinx_delete('test_index', 5);

SELECT * FROM sphinx_select('test_index', 'database', 'user_id = 12', 'weight DESC, id DESC', 0, 100, 'sort_method = ''kbuffer''');

SELECT sphinx_snippet('test_index', 'database', 'Text about databases', '<mark>', '</mark>');

SELECT sphinx_snippet_options('test_index', 'database', 'Text about databases',
  ARRAY['before_match', '<mark>',
        'after_match', '</mark>',
        'limit', '100',
        'allow_empty', 'y']);

SELECT sphinx_delete('test_index', 6);

