# CC = gcc
# CPP = g++
# RM = rm -f
# MKDIR = mkdir -p

# ifeq ($(debug),1)
# 	CFLAGS = -I ./srcs -O0 -g -Wall -fexceptions -fPIC -DLINUX -lpthread
# else
# 	CFLAGS = -I ./srcs -O2 -fexceptions -fPIC -DLINUX -lpthread
# endif

# BINPATH = ./bin
# CPPPATH = ./srcs
# OBJSPATH = ./objs
# CPPFILES = $(foreach dir, $(CPPPATH), $(wildcard $(dir)/*.cpp))
# $(info cppfile $(CPPFILES))
# OBJS = $(patsubst %.cpp, $(OBJSPATH)/%.o,$(notdir $(CPPFILES)))
# $(info OBJS $(OBJS))
# TARGET = $(BINPATH)/test

# $(OBJSPATH) $(BINPATH):
# 	$(MKDIR) $@

# $(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
# 	$(CPP) -c $(CFLAGS) $< -o $@

# $(TARGET): $(OBJS) | $(BINPATH)
# 	$(CPP) $(CFLAGS) $(OBJS) -o $(TARGET)

# .PHONY : clean
# clean:
# 	-$(RM) $(TARGET)
# 	-$(RM) $(OBJSPATH)/*.o 

CC = gcc
CPP = g++
RM = rm -f
MKDIR = mkdir -p

ifeq ($(debug),1)
    CFLAGS = -I ./srcs -O0 -g -Wall -fexceptions -fPIC -DLINUX -lpthread
else
    CFLAGS = -I ./srcs -O2 -fexceptions -fPIC -DLINUX -lpthread
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

# 生成目标可执行文件的规则
$(SERVER): $(OBJS) | $(BINPATH) $(CLIENT)
	$(CPP) $(CFLAGS) $(OBJS) $(SERVERFILE) -o $(SERVER)

$(CLIENT): $(OBJS) | $(BINPATH)
	$(CPP) $(CFLAGS) $(OBJS) $(CLIENTFILE) -o $(CLIENT)

$(OBJSPATH) $(BINPATH):
	$(MKDIR) $@

# 生成 .o 文件的规则
$(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
	$(CPP) -c $(CFLAGS) $< -o $@


.PHONY : clean
clean:
	-$(RM) $(SERVER)
	-$(RM) $(CLIENT)
	-$(RM) $(OBJSPATH)/*.o 