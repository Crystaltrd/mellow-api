CFLAGS=-g -Wall -Wextra `pkg-config --static --cflags libargon2 kcgi-html kcgi-json sqlbox`
LDFLAGS=--static `pkg-config --static --libs libargon2 kcgi-html kcgi-json sqlbox`
LDFLAGS_LINUX= `pkg-config --static --libs libmd libbsd`
DESTDIR=/var/www/cgi-bin
USER=www
GROUP=www

all: build/borrow build/delete build/hit build/add build/edit build/query build/auth build/deauth build/signup build/search build/database.db build/me
install:install-borrow install-delete install-me install-hit install-edit install-add install-auth install-deauth install-query install-signup install-search install-db

build/add.o: src/add.c
	${CC} ${CFLAGS} -c -o build/add.o src/add.c
build/add: build/add.o
	${CC} -o build/add build/add.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-add: build/add
	install -o ${USER} -g ${GROUP} -m 0500 build/add ${DESTDIR}/add


build/auth.o: src/auth.c
	${CC} ${CFLAGS} -c -o build/auth.o src/auth.c
build/auth: build/auth.o
	${CC} -o build/auth build/auth.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-auth: build/auth
	install -o ${USER} -g ${GROUP} -m 0500 build/auth ${DESTDIR}/auth


build/borrow.o: src/borrow.c
	${CC} ${CFLAGS} -c -o build/borrow.o src/borrow.c
build/borrow: build/borrow.o
	${CC} -o build/borrow build/borrow.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-borrow: build/borrow
	install -o ${USER} -g ${GROUP} -m 0500 build/borrow ${DESTDIR}/borrow


build/deauth.o: src/deauth.c
	${CC} ${CFLAGS} -c -o build/deauth.o src/deauth.c
build/deauth: build/deauth.o
	${CC} -o build/deauth build/deauth.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-deauth: build/deauth
	install -o ${USER} -g ${GROUP} -m 0500 build/deauth ${DESTDIR}/deauth


build/delete.o: src/delete.c
	${CC} ${CFLAGS} -c -o build/delete.o src/delete.c
build/delete: build/delete.o
	${CC} -o build/delete build/delete.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-delete: build/delete
	install -o ${USER} -g ${GROUP} -m 0500 build/delete ${DESTDIR}/delete


build/edit.o: src/edit.c
	${CC} ${CFLAGS} -c -o build/edit.o src/edit.c
build/edit: build/edit.o
	${CC} -o build/edit build/edit.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-edit: build/edit
	install -o ${USER} -g ${GROUP} -m 0500 build/edit ${DESTDIR}/edit


build/me.o: src/me.c
	${CC} ${CFLAGS} -c -o build/me.o src/me.c
build/me: build/me.o
	${CC} -o build/me build/me.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-me: build/me
	install -o ${USER} -g ${GROUP} -m 0500 build/me ${DESTDIR}/me


build/hit.o: src/hit.c
	${CC} ${CFLAGS} -c -o build/hit.o src/hit.c
build/hit: build/hit.o
	${CC} -o build/hit build/hit.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-hit: build/hit
	install -o ${USER} -g ${GROUP} -m 0500 build/hit ${DESTDIR}/hit


build/query.o: src/query.c
	${CC} ${CFLAGS} -c -o build/query.o src/query.c
build/query: build/query.o
	${CC} -o build/query build/query.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-query: build/query
	install -o ${USER} -g ${GROUP} -m 0500 build/query ${DESTDIR}/query


build/return.o: src/return.c
	${CC} ${CFLAGS} -c -o build/return.o src/return.c
build/return: build/return.o
	${CC} -o build/return build/return.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-return: build/return
	install -o ${USER} -g ${GROUP} -m 0500 build/return ${DESTDIR}/return


build/signup.o: src/signup.c
	${CC} ${CFLAGS} -c -o build/signup.o src/signup.c
build/signup: build/signup.o
	${CC} -o build/signup build/signup.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-signup: build/signup
	install -o ${USER} -g ${GROUP} -m 0500 build/signup ${DESTDIR}/signup


build/search.o: src/search.c
	${CC} ${CFLAGS} -c -o build/search.o src/search.c
build/search: build/search.o
	${CC} -o build/search build/search.o ${LDFLAGS} ${LDFLAGS_LINUX}
install-search: build/search
	install -o ${USER} -g ${GROUP} -m 0500 build/search ${DESTDIR}/search


build/database.db: misc/database-scheme.sql
	[ -f build/database.db ] && rm build/database.db || echo "Skipping db"
	sqlite3 build/database.db < misc/database-scheme.sql
install-db: build/database.db
	[ -d ${DESTDIR}/db ] ||  mkdir -p ${DESTDIR}/db
	chown ${USER}:${GROUP} ${DESTDIR}/db
	chmod 0700 ${DESTDIR}/db
	install -o ${USER} -g ${GROUP} -m 0600 build/database.db ${DESTDIR}/db


clean:
	rm -rfv build/*
