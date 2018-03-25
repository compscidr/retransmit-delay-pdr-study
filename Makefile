all: deps client server
deps:
	sudo apt-get install libboost-all-dev gnuplot
client:	client.cc
	g++ client.cc -lboost_system -lboost_program_options -lpthread -o client
server: server.cc
	g++ server.cc -lboost_system -lpthread -o server
clean:
	rm -rf client server
data:
	dd if=/dev/urandom of=random.bin bs=1048576 count=100
