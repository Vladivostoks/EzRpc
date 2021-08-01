#TOOL_CHAIN = /usr/local/bin/gcc-11
TOOL_CHAIN = clang

CPP_FLAG += -std=c++14 #-stdlib=libc++
LD_FLAG += -lstdc++

all:client server

%.o:%.cpp
	$(TOOL_CHAIN) $(CPP_FLAG) -c $< -o $@

%.o:%.c
	$(TOOL_CHAIN) $(C_FLAG) -c $< -o $@

client: client.o api_cluster.o
	$(TOOL_CHAIN) $(LD_FLAG) $^ -o $@

server: server.o api_cluster.o
	$(TOOL_CHAIN) $(LD_FLAG) $^ -o $@

.PHONY: clean

clean: 
	-rm client server *.o

