#TOOL_CHAIN = /usr/local/bin/gcc-11
TOOL_CHAIN = clang

CPP_FLAG += -std=c++14 #-stdlib=libc++
LD_FLAG += -lstdc++

all:client server

%.o:%.cpp
	$(TOOL_CHAIN) $(CPP_FLAG) -c $< -o $@ -I.

%.o:%.c
	$(TOOL_CHAIN) $(C_FLAG) -c $< -o $@ -I.

%-init.c:%.c
	$(TOOL_CHAIN) $(C_FLAG) -Xclang -ast-print -fsyntax-only -c $< -I. > $@

%-init.cpp:%.cpp
	$(TOOL_CHAIN) $(CPP_FLAG) -Xclang -ast-print -fsyntax-only -c $< -I. > $@

client: demo/client/client.o demo/server/dev/api_cluster.o
	$(TOOL_CHAIN) $(LD_FLAG) $^ -o $@

server: demo/server/core/server.o
	$(TOOL_CHAIN) $(LD_FLAG) $^ -o $@

.PHONY: clean

clean: 
	-rm client server demo/server/core/*.o demo/server/dev/*.o demo/client/*.o

