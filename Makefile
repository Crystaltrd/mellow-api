all: query auth

query: query.o
	cc -static -o query query.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`

query.o:
	cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o query.o src/query.c

auth: auth.o
	cc -static -o auth auth.o `pkg-config --static --libs kcgi-html kcgi-json sqlbox`

auth.o:
	cc `pkg-config --static --cflags kcgi-html kcgi-json sqlbox` -c -o auth.o src/auth.c