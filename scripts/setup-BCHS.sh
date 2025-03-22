#! /bin/sh
[ -d bins ] || mkdir bins
cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o query.o src/query.c
cc -static -o bins/query query.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`
cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o auth.o src/auth.c
cc -static -o bins/auth auth.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`
[ -d /var/www/cgi-bin/mellow ] || doas mkdir /var/www/cgi-bin/mellow
[ -d /var/www/cgi-bin/mellow/db ] || doas mkdir /var/www/cgi-bin/mellow/db
doas chown www:www /var/www/cgi-bin/mellow/db
doas chmod 0700 /var/www/cgi-bin/mellow/db
doas install -o www -g www -m 0500 bins/query /var/www/cgi-bin/mellow
doas install -o www -g www -m 0500 bins/auth /var/www/cgi-bin/mellow
rm -rfv database.db
sqlite3 database.db < misc/database-scheme.sql
doas install -o www -g www -m 0600 database.db /var/www/cgi-bin/mellow/db
doas rcctl enable slowcgi
doas rcctl start slowcgi
doas rcctl check slowcgi
doas rcctl restart httpd
rm -rfv *.o