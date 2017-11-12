server: defs.h Server.h
	gcc -Wall -o server server.c -lpthread
clean:
	rm server
