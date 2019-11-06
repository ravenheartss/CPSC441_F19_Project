all: User Server

User: User.cpp
	g++ -g -std=c++11 -o User User.cpp

Server: Server.cpp wrdgen.cpp Game.cpp
	g++ -Ofast -w -g -std=c++11 -lpthread -c Server.cpp
	g++ -Ofast -w -g -std=c++11 -lpthread -c wrdgen.cpp
	g++ -Ofast -w -g -std=c++11 -lpthread -c Game.cpp
	g++ -Ofast -w -std=c++11 -lpthread -o Server Server.o wrdgen.o Game.o

clean: 
	rm -rf User Server.o wrdgen.o Game.o Server *.dSYM
