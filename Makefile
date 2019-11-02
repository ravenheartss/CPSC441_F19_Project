all: User Server

User: User.cpp
	g++ -g -std=c++11 -o User User.cpp

Server: Server.cpp wrdgen.cpp Game.cpp
	g++ -g -std=c++11 -c Server.cpp
	g++ -g -std=c++11 -c wrdgen.cpp
	g++ -g -std=c++11 -c Game.cpp
	g++ -std=c++11 -o Server Server.o wrdgen.o Game.o
	g++ -std=c++11 -o User User.cpp

clean: 
	rm -rf User Server.o wrdgen.o Game.o Server *.dSYM
