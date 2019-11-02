all: User Server

User: User.cpp
	g++ -g -std=c++11 -o User User.cpp

Server: Server.cpp wrdgen.cpp Game.cpp
	g++ -g -c Server.cpp
	g++ -g -c wrdgen.cpp
	g++ -g -c Game.cpp
	g++ -o Server Server.o wrdgen.o Game.o
	g++ -o User User.cpp

clean: 
	rm -rf User Server.o wrdgen.o Game.o Server *.dSYM
