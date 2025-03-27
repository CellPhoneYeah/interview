CC = gcc
CPP = g++
RM = rm -f
MKDIR = mkdir -p
3RD_LIBS_PATH = ../libs

INCLUDE_ARGS = -I ../net/srcs \
	-I ../watchdog/srcs \
	-I ../3rd/spdlog/include \
	-I../include/spdlog \
	-I../messageQueue/srcs

LIB_PATH = -L../watchdog/objs \
	-L../net/objs


LIB_ARGS = -lspdlog \
	-lpthread \
	-lwatchdog \
	-lepollnet

ifeq ($(debug),1)
    CFLAGS = $(INCLUDE_ARGS) -O0 -g -Wall -fexceptions -fPIC -DLINUX -L$(3RD_LIBS_PATH)  
else
    CFLAGS = $(INCLUDE_ARGS) -O2 -fexceptions -fPIC -DLINUX -L$(3RD_LIBS_PATH) $(LIB_PATH) $(LIB_ARGS)
endif

BINPATH = ./bin
CPPPATH = ./
OBJSPATH = ./objs
CPPFILES = $(foreach dir, $(CPPPATH), $(wildcard $(dir)/*.cpp))
$(info cppfile $(CPPFILES))
OBJS = $(patsubst %.cpp, $(OBJSPATH)/%.o,$(notdir $(CPPFILES)))
$(info OBJS $(OBJS))
TARGETFILE = ./entrance.cpp
TARGET = $(BINPATH)/entrance

# 生成目标可执行文件的规则
$(TARGET): $(OBJS) $(TARGETFILE) | $(BINPATH)
	$(CPP) $(CFLAGS) $(TARGETFILE) -o $(TARGET) $(LIB_PATH) $(LIB_ARGS)

$(OBJSPATH) $(BINPATH):
	$(MKDIR) $@

# 生成 .o 文件的规则
$(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
	$(CPP) -c $(CFLAGS) $< -o $@


.PHONY : clean
clean:
	-$(RM) $(TARGET)
	-$(RM) $(OBJSPATH)/*.o 