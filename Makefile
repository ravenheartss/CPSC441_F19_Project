all: User Server

User: User.cpp
	g++ -g -o User User.cpp

Server: Server.cpp wrdgen.cpp
	g++ -g -o Server Server.cpp
	g++ -g -o wrdgen wrdgen.cpp

clean: 
	rm -f User Server wrdgen
