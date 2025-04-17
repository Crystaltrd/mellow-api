CFLAGS=-g -Wall -Wextra `pkg-config --static --cflags libargon2 kcgi-html kcgi-json sqlbox`
LDFLAGS=--static `pkg-config --static --libs libargon2 kcgi-html kcgi-json sqlbox`
LDFLAGS_LINUX= `pkg-config --static --libs libmd libbsd`
PBNIX_HTML=${HOME}/public_html/mellow
DESTDIR=/var/www/cgi-bin/mellow


all: build/edit build/query build/auth build/deauth build/signup build/search database.db
install: install-noreplace install-db
install-pubnix: install-pubnix-noreplace install-db-pubnix
install-noreplace: install-edit install-auth install-deauth install-query install-signup install-search
install-pubnix-noreplace: install-edit-pubnix install-auth-pubnix install-deauth-pubnix install-query-pubnix install-signup-pubnix install-search-pubnix

build/auth.o: src/auth.c
	${CC} ${CFLAGS} -c -o build/auth.o src/auth.c
build/auth: build/auth.o
	${CC} -o build/auth build/auth.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-auth-pubnix: build/auth
	install -m 0755 build/auth ${PBNIX_HTML}/auth.cgi
install-auth: build/auth
	install -o www -g www -m 0500 build/auth ${DESTDIR}/auth
clean-auth: build/auth.o build/auth
	rm build/auth
	rm build/auth.o


build/deauth.o: src/deauth.c
	${CC} ${CFLAGS} -c -o build/deauth.o src/deauth.c
build/deauth: build/deauth.o
	${CC} -o build/deauth build/deauth.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-deauth-pubnix: build/deauth
	install -m 0755 build/deauth ${PBNIX_HTML}/deauth.cgi
install-deauth: build/deauth
	install -o www -g www -m 0500 build/deauth ${DESTDIR}/deauth
clean-deauth: build/deauth.o build/deauth
	rm build/deauth
	rm build/deauth.o

build/edit.o: src/edit.c
	${CC} ${CFLAGS} -c -o build/edit.o src/edit.c
build/edit: build/edit.o
	${CC} -o build/edit build/edit.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-edit-pubnix: build/edit
	install -m 0755 build/edit ${PBNIX_HTML}/edit.cgi
install-edit: build/edit
	install -o www -g www -m 0500 build/edit ${DESTDIR}/edit
clean-edit: build/edit.o build/edit
	rm build/edit
	rm build/edit.o

build/query.o: src/query.c
	${CC} ${CFLAGS} -c -o build/query.o src/query.c
build/query: build/query.o
	${CC} -o build/query build/query.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-query-pubnix: build/query
	install -m 0755 build/query ${PBNIX_HTML}/query.cgi
install-query: build/query
	install -o www -g www -m 0500 build/query ${DESTDIR}/query
clean-query: build/query.o build/query
	rm build/query
	rm build/query.o

build/signup.o: src/signup.c
	${CC} ${CFLAGS} -c -o build/signup.o src/signup.c
build/signup: build/signup.o
	${CC} -o build/signup build/signup.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-signup-pubnix: build/signup
	install -m 0755 build/signup ${PBNIX_HTML}/signup.cgi
install-signup: build/signup
	install -o www -g www -m 0500 build/signup ${DESTDIR}/signup
clean-signup: build/signup.o build/signup
	rm build/signup
	rm build/signup.o

build/search.o: src/search.c
	${CC} ${CFLAGS} -c -o build/search.o src/search.c
build/search: build/search.o
	${CC} -o build/search build/search.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-search-pubnix: build/search
	install -m 0755 build/search ${PBNIX_HTML}/search.cgi
install-search: build/search
	install -o www -g www -m 0500 build/search ${DESTDIR}/search
clean-search: build/search.o build/search
	rm build/search
	rm build/search.o

build/database.db: misc/database-scheme.sql
	[ -f build/database.db ] && rm build/database.db || echo "Skipping db"
	sqlite3 build/database.db < misc/database-scheme.sql
install-db-pubnix: build/database.db
	[ -d ${PBNIX_HTML}/db ] ||  mkdir -p ${PBNIX_HTML}/db
	install -m 0666 build/database.db ${PBNIX_HTML}/db
install-db: build/database.db
	[ -d ${DESTDIR}/db ] ||  mkdir -p ${DESTDIR}/db
	chown www:www ${DESTDIR}/db
	chmod 0700 ${DESTDIR}/db
	install -o www -g www -m 0600 build/database.db ${DESTDIR}/db
clean-db: build/database.db
	rm build/database.db

clean: clean-auth clean-deauth clean-db clean-edit clean-query clean-signup clean-search
