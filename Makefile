CFLAGS=-g -Wall -Wextra `pkg-config --cflags kcgi-html kcgi-json sqlbox`
LDFLAGS=`pkg-config --libs kcgi-html kcgi-json sqlbox`
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
	${CC} -o auth auth.o ${LDFLAGS}
query: query.o
	${CC} -o query query.o ${LDFLAGS}
auth.o:
	${CC} ${CFLAGS} -c -o auth.o src/auth.c
query.o:
	${CC} ${CFLAGS} -c -o query.o src/query.c
database.db: misc/database-scheme.sql
	sqlite3 database.db < misc/database-scheme.sql
clean:
	rm -rfv auth.o
	rm -rfv query.o
	rm -rfv query
	rm -rfv auth
	rm -rfv database.db