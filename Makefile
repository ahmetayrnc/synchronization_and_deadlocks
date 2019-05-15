all: libralloc.a  app test

libralloc.a:  ralloc.c
	gcc -g -Wall -c ralloc.c
	ar -cvq libralloc.a ralloc.o
	ranlib libralloc.a

app: app.c
	gcc -g -Wall -o app app.c -L. -lralloc -lpthread

test: test.c
	gcc -g -Wall -o test test.c -L. -lralloc -lpthread

clean: 
	rm -fr *.o *.a *~ a.out app test ralloc.o ralloc.a libralloc.a