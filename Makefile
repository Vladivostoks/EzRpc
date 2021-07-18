
CPP_FLAG += -std=c++14 -stdlib=libc++
LD_FLAG += 

all:client server

%.o:%.cpp
	g++ $(CPP_FLAG) -c $< -o $@

%.o:%.c
	gcc $(C_FLAG) -c $< -o $@

client: client.o api_cluster.o
	g++ $(LD_FLAG) $^ -o $@

server: server.o api_cluster.o
	g++ $(LD_FLAG) $^ -o $@

.PHONY: clean

clean: 
	-rm client server *.o

