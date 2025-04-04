CFLAGS=-g -Wall -Wextra `pkg-config --static --cflags kcgi-html kcgi-json sqlbox`
LDFLAGS=`pkg-config --static --libs kcgi-html kcgi-json sqlbox`
PBNIX_HTML=${HOME}/public_html/mellow
DESTDIR=/var/www/cgi-bin/mellow
all: query auth database.db


install: query deauth auth database.db
	[ -d ${DESTDIR}/db ] ||  mkdir -p ${DESTDIR}/db
	chown www:www ${DESTDIR}/db
	chmod 0700 ${DESTDIR}/db
	test [ "${REPLACEDB}" = "true" ] && install -o www -g www -m 0600 database.db ${DESTDIR}/db
	install -o www -g www -m 0500 query ${DESTDIR}/query
	install -o www -g www -m 0500 auth ${DESTDIR}/auth
	install -o www -g www -m 0500 deauth ${DESTDIR}/deauth

install-pubnix: query deauth auth database.db
	[ -d ${PBNIX_HTML}/db ] ||  mkdir -p ${PBNIX_HTML}/db
	test [ "${REPLACEDB}" = "true" ] && install -m 0666 database.db ${PBNIX_HTML}/db
	install -m 0755 query ${PBNIX_HTML}/query.cgi
	install -m 0755 auth ${PBNIX_HTML}/auth.cgi
	install -m 0755 deauth ${PBNIX_HTML}/deauth.cgi

auth: auth.o
	${CC} --static -o auth auth.o ${LDFLAGS}

deauth: deauth.o
	${CC} --static -o deauth deauth.o ${LDFLAGS}

query: query.o
	${CC} --static -o query query.o ${LDFLAGS}

auth.o: src/auth.c
	${CC} ${CFLAGS} -c -o auth.o src/auth.c

deauth.o: src/deauth.c
	${CC} ${CFLAGS} -c -o deauth.o src/deauth.c

query.o: src/query.c
	${CC} ${CFLAGS} -c -o query.o src/query.c

database.db: misc/database-scheme.sql
	rm database.db
	sqlite3 database.db < misc/database-scheme.sql
clean:
	rm auth.o
	rm deauth.o
	rm query.o
	rm query
	rm auth