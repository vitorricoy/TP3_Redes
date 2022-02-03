all:
	g++ -Wall -c common.cpp
	g++ -Wall -c estruturas.cpp
	g++ -Wall emitter.cpp common.o -o emitter
	g++ -Wall exhibitor.cpp common.o -o exhibitor
	g++ -Wall server.cpp common.o estruturas.o -pthread -o server

clean:
	rm common.o exhibitor emitter server
