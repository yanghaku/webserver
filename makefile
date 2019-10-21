server: webserver.o http.h http.o
	gcc webserver.o http.o http.h -o server -O2 -m64
webserver.o: webserver.c http.h
	gcc webserver.c -c -o webserver.o -O2 -m64
http.o: http.c http.h
	gcc http.c -c -o http.o -O2 -w -m64


clean:
	rm *.o server
