[ -d bins ] || mkdir bins
cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o query.o src/query.c
cc -static -o bins/query.cgi query.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`
[ -d ~/public_html/mellow ] || mkdir ~/public_html/mellow
[ -d ~/public_html/mellow/db ] ||  mkdir ~/public_html/mellow/db
chmod 0777 ~/public_html/mellow/db
install -m 0755 bins/query.cgi ~/public_html/mellow
rm -rfv database.db
sqlite3 database.db < misc/database-scheme.sql
install -m 0666 database.db ~/public_html/mellow/db
rm -rfv *.o
