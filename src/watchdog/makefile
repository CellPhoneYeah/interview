CC = gcc
CPP = g++
RM = rm -f
MKDIR = mkdir -p
3RD_LIBS_PATH = ../libs

LIB_PATH = -L../net/objs

ifeq ($(debug),1)
    CFLAGS = -I ../3rd/spdlog/include -I../include/spdlog -O0 -g -Wall -fexceptions -fPIC -DLINUX -lpthread -L$(3RD_LIBS_PATH) $(LIB_PATH) -lspdlog -lepollnet
else
    CFLAGS = -I ../3rd/spdlog/include -I../include/spdlog -O2 -fexceptions -fPIC -DLINUX -lpthread -L$(3RD_LIBS_PATH) $(LIB_PATH) -lspdlog -lepollnet
endif

BINPATH = ./bin
OBJSPATH = ./objs
CPPPATH = ./srcs
CPPFILES = $(foreach dir, $(CPPPATH), $(wildcard $(dir)/*.cpp))
$(info cppfile $(CPPFILES))
OBJS = $(patsubst %.cpp, $(OBJSPATH)/%.o,$(notdir $(CPPFILES)))
$(info OBJS $(OBJS))
TARGETFILE = ./test.cpp
TARGET = $(BINPATH)/test
LIB = $(OBJSPATH)/libwatchdog.a

# 生成目标可执行文件的规则
$(TARGET): $(OBJS) $(TARGETFILE) $(LIB)| $(BINPATH)
	$(CPP) $(CFLAGS) -I./srcs $(OBJS) $(TARGETFILE) -o $(TARGET)

$(LIB): $(OBJS) | $(OBJSPATH)
	ar rcs $@ $(OBJS)
	ranlib $@

$(OBJSPATH) $(BINPATH):
	$(MKDIR) $@

# 生成 .o 文件的规则
$(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
	$(CPP) -c $(CFLAGS) $< -o $@


.PHONY : clean
clean:
	-$(RM) $(TARGET)
	-$(RM) $(OBJSPATH)/*.o 
	-$(RM) $(OBJSPATH)/*.a
	-$(RM) $(OBJSPATH)/*.so