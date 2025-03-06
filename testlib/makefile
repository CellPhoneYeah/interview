CC = gcc
CPP = g++
RM = rm -f

CFLAGS = -g -I../include/spdlog -I../3rd/spdlog/include -L../libs -lspdlog

SVR = ./svr
CLE = ./cle

$(SVR):server.cpp | $(CLE)
	$(CPP) $(CFLAGS) server.cpp -o $(SVR)

$(CLE):client.cpp
	$(CPP) $(CFLAGS) client.cpp -o $(CLE)

.PHONY : clean
clean:
	$(RM) $(SVR)
	$(RM) $(CLE)