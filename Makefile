all:
	g++ -Wall -c common.c
	g++ -Wall emissor.c common.o -o emissor
	g++ -Wall exibidor.c common.o -o exibidor
	g++ -Wall server.c common.o -o server

clean:
	rm common.o exibidor emissor server
