PGSQL_CFLAGS=-I `pg_config --includedir-server`
MYSQL_CFLAGS=`mysql_config --cflags`

SRC=pg_sphinx.c sphinx.c stringbuilder.c log.c
OBJ=$(SRC:.c=.lo)

libpgsphinx.la: $(OBJ)
	libtool link gcc -module -g3 -o libpgsphinx.la $(OBJ) `mysql_config --libs_r` -rpath `pwd`

.c.lo:
	libtool compile gcc -Wall -Werror -g3 -c $(PGSQL_CFLAGS) $(MYSQL_CFLAGS) $<

clean:
	rm -rf *.o *.lo .lib

.SUFFIXES: .c .lo .o

