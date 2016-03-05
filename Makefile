MODULE_big = sphinx
OBJS = pg_sphinx.o sphinx.o pstring.o stringbuilder.o error.o
EXTENSION = sphinx
DATA = sphinx--0.3.sql sphinx--0.2--0.3.sql sphinx--0.1--0.2.sql

PG_CPPFLAGS=`mysql_config --cflags`
SHLIB_LINK=`mysql_config --libs_r`

REGRESS=basic nulls

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

deb-package:
	echo 'PostgreSQL connector to SphinxSearch server' > description-pak
	mkdir doc-pak
	cp README.org doc-pak
	checkinstall -D \
		--install=no \
		--backup=no \
		--pkgname pg-sphinx \
		--pkgversion 0.3 \
		--pkglicense MIT \
		--pkggroup database \
		--maintainer 'Andrey Kutejko \<andy128k@gmail.com\>' \
		-y \
		make install
	rm description-pak
	rm -rf doc-pak

