#! /bin/sh
cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o cgi.o main.c
cc -static -o api cgi.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`
[ -d /var/www/cgi-bin/mellow ] || doas mkdir /var/www/cgi-bin/mellow
[ -d /var/www/cgi-bin/mellow/db ] || doas mkdir /var/www/cgi-bin/mellow/db
doas chown www:www /var/www/cgi-bin/mellow/db
doas chmod 0700 /var/www/cgi-bin/mellow/db
doas install -o www -g www -m 0500 api /var/www/cgi-bin/mellow
rm -rfv database.db
sqlite3 database.db < database-scheme.sql
doas install -o www -g www -m 0600 database.db /var/www/cgi-bin/mellow/db
doas rcctl enable slowcgi
doas rcctl start slowcgi
doas rcctl check slowcgi
doas rcctl restart httpd