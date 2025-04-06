CFLAGS=-g -Wall -Wextra `pkg-config --static --cflags kcgi-html kcgi-json sqlbox`
LDFLAGS=`pkg-config --static --libs kcgi-html kcgi-json sqlbox`
PBNIX_HTML=${HOME}/public_html/mellow
DESTDIR=/var/www/cgi-bin/mellow
all: query auth deauth database.db
install: install-auth install-deauth install-query install-db
install-pubnix: install-auth-pubnix install-deauth-pubnix install-query-pubnix install-db-pubnix
install-noreplace: install-auth install-deauth install-query install-db
install-pubnix-noreplace: install-auth-pubnix install-deauth-pubnix install-query-pubnix install-db-pubnix

auth.o: src/auth.c
	${CC} ${CFLAGS} -c -o auth.o src/auth.c
auth: auth.o
	${CC} --static -o auth auth.o ${LDFLAGS}
install-auth-pubnix: auth
	install -m 0755 auth ${PBNIX_HTML}/auth.cgi
install-auth: auth
	install -o www -g www -m 0500 auth ${DESTDIR}/auth


deauth.o: src/deauth.c
	${CC} ${CFLAGS} -c -o deauth.o src/deauth.c
deauth: deauth.o
	${CC} --static -o deauth deauth.o ${LDFLAGS}
install-deauth-pubnix: deauth
	install -m 0755 deauth ${PBNIX_HTML}/deauth.cgi
install-deauth: deauth
	install -o www -g www -m 0500 deauth ${DESTDIR}/deauth


query.o: src/query.c
	${CC} ${CFLAGS} -c -o query.o src/query.c
query: query.o
	${CC} --static -o query query.o ${LDFLAGS}
install-query-pubnix: query
	install -m 0755 query ${PBNIX_HTML}/query.cgi
install-query: query
	install -o www -g www -m 0500 query ${DESTDIR}/query



database.db: misc/database-scheme.sql
	rm database.db
	sqlite3 database.db < misc/database-scheme.sql
install-db-pubnix: database.db
	[ -d ${PBNIX_HTML}/db ] ||  mkdir -p ${PBNIX_HTML}/db
	install -m 0666 database.db ${PBNIX_HTML}/db
install-db: database.db
	[ -d ${DESTDIR}/db ] ||  mkdir -p ${DESTDIR}/db
	chown www:www ${DESTDIR}/db
	chmod 0700 ${DESTDIR}/db
	install -o www -g www -m 0600 database.db ${DESTDIR}/db

clean:
	rm auth.o
	rm deauth.o
	rm query.o
	rm query
	rm auth