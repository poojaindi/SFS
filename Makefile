all: server

server: server.c multicasts.c
	gcc -o server.exe server.c multicasts.c -lpthread

clean: 
	rm *.exe
