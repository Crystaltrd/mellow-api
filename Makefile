CFLAGS=-g -Wall -Wextra `pkg-config --static --cflags kcgi-html kcgi-json sqlbox`
LDFLAGS=`pkg-config --static --libs kcgi-html kcgi-json sqlbox`
PBNIX_HTML=${HOME}/public_html/mellow
DESTDIR=/var/www/cgi-bin/mellow
all: query auth database.db


install: query auth database.db
	[ -d ${DESTDIR}/db ] ||  mkdir -p ${DESTDIR}/db
	chown www:www ${DESTDIR}/db
	chmod 0700 ${DESTDIR}/db
	install -o www -g www -m 0600 database.db ${DESTDIR}/db
	install -o www -g www -m 0500 query ${DESTDIR}/query
	install -o www -g www -m 0500 auth ${DESTDIR}/auth
install-pubnix: query auth database.db
	[ -d ${PBNIX_HTML}/db ] ||  mkdir -p ${PBNIX_HTML}/db
	install -m 0655 database.db ${PBNIX_HTML}/db
	install -m 0755 query ${PBNIX_HTML}/query.cgi
	install -m 0755 auth ${PBNIX_HTML}/auth.cgi
auth: auth.o
	${CC} --static -o auth auth.o ${LDFLAGS}
query: query.o
	${CC} --static -o query query.o ${LDFLAGS}
auth.o:
	${CC} ${CFLAGS} -c -o auth.o src/auth.c
query.o:
	${CC} ${CFLAGS} -c -o query.o src/query.c
database.db: misc/database-scheme.sql
	rm database.db
	sqlite3 database.db < misc/database-scheme.sql
clean:
	rm auth.o
	rm query.o
	rm query
	rm auth