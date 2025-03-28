CC = gcc
CPP = g++
RM = rm -f
MKDIR = mkdir -p
3RD_LIBS_PATH = ../libs

ifeq ($(debug),1)
    CFLAGS = -I ./srcs -I ../3rd/spdlog/include -I../include/spdlog -O0 -g -Wall -fexceptions -fPIC -DLINUX -lpthread -L$(3RD_LIBS_PATH) -lspdlog
else
    CFLAGS = -I ./srcs -I ../3rd/spdlog/include -I../include/spdlog -O2 -fexceptions -fPIC -DLINUX -lpthread -L$(3RD_LIBS_PATH) -lspdlog
endif

BINPATH = ./bin
CPPPATH = ./srcs
OBJSPATH = ./objs
CPPFILES = $(foreach dir, $(CPPPATH), $(wildcard $(dir)/*.cpp))
$(info cppfile $(CPPFILES))
OBJS = $(patsubst %.cpp, $(OBJSPATH)/%.o,$(notdir $(CPPFILES)))
$(info OBJS $(OBJS))
SERVERFILE = ./EpollServerTest.cpp
CLIENTFILE = ./EpollClientTest.cpp
SERVER = $(BINPATH)/server
CLIENT = $(BINPATH)/client
LIB = $(OBJSPATH)/libepollnet.a

# 生成目标可执行文件的规则
$(SERVER): $(OBJS) $(SERVERFILE) | $(BINPATH) $(CLIENT)  $(LIB)
	$(CPP) $(CFLAGS) $(OBJS) $(SERVERFILE) -o $(SERVER)

$(CLIENT): $(OBJS) $(CLIENTFILE)| $(BINPATH)
	$(CPP) $(CFLAGS) $(OBJS) $(CLIENTFILE) -o $(CLIENT)

$(LIB): $(OBJS) | $(OBJSPATH)
	ar rcs $@ $(OBJS)
	ranlib $@

$(OBJSPATH) $(BINPATH):
	$(MKDIR) $@

# 生成 .o 文件的规则
$(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
	$(CPP) -c $(CFLAGS) -shared $< -o $@


.PHONY : clean
clean:
	-$(RM) $(SERVER)
	-$(RM) $(CLIENT)
	-$(RM) $(OBJSPATH)/*.o 
	-$(RM) $(OBJSPATH)/*.so
	-$(RM) $(OBJSPATH)/*.a