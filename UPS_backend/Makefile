CC=g++
CFLAGS=-g
PROTO_CC = ./protobuf/world_ups.pb.cc 
PROTO_H = ./protobuf/world_ups.pb.h 
GPB = `pkg-config --cflags --libs protobuf`
VERSION = -std=c++11
THREAD = -pthread
PQXX = -lpqxx -lpq
CPPFILES = ups_client.cpp socket.cpp
HEADERS = ups_client.hpp socket.hpp threadpool.hpp send_recv.hpp

all: world_server ups_client amazon_server

world_server: world_server.cpp socket.hpp socket.cpp threadpool.hpp
	$(CC) $(CFLAGS) -o world_server world_server.cpp socket.cpp $(PQXX) $(THREAD)

ups_client:	$(HEADERS) $(PROTO_H) $(CPPFILES)
	$(CC) $(CFLAGS)  -o ups_client $(CPPFILES) $(PROTO_CC) $(GPB) $(PQXX) $(THREAD) $(VERSION)

amazon_server: amazon.cpp socket.hpp socket.cpp threadpool.hpp
	$(CC) $(CFLAGS) -o amazon_server amazon.cpp socket.cpp $(PQXX) $(THREAD)

.PHONY:
	clean

clean:
	rm -rf *.o world_server ups_client amazon_server
