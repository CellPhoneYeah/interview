# CC = gcc
# CPP = g++
# RM = rm -f
# MKDIR = mkdir -p

# SPDLOG_ROOT = ./spdlog

# INCLUDE_PATH = include bench example tests
# INCLUDE_FLAGS = $(addprefix -I$(SPDLOG_ROOT)/, $(INCLUDE_PATH))
# $(info include flags $(INCLUDE_FLAGS))

# ifeq ($(debug),1)
#     CFLAGS = $(INCLUDE_FLAGS) -O0 -g -Wall -fexceptions -fPIC -lpthread -DSPDLOG_COMPILED_LIB
# else
#     CFLAGS = $(INCLUDE_FLAGS) -O2 -fexceptions -fPIC -lpthread -DSPDLOG_COMPILED_LIB
# endif

# OUTPATH = ../libs
# CPPPATH = $(SPDLOG_ROOT)/src
# OBJSPATH = $(SPDLOG_ROOT)/objs
# $(info file path $(CPPPATH))
# CPPFILES = $(foreach dir, $(CPPPATH), $(wildcard $(dir)/*.cpp))
# $(info cppfile $(CPPFILES))
# OBJS = $(patsubst %.cpp, $(OBJSPATH)/%.o,$(notdir $(CPPFILES)))
# $(info OBJS $(OBJS))

# TARGET = $(OUTPATH)/libspdlog.so

# # 生成目标可执行文件的规则
# $(TARGET): $(OBJS) | $(OUTPATH) $(OBJSPATH)
# 	$(CPP) -shared -o $@ $^

# $(OUTPATH) $(OBJSPATH):
# 	$(MKDIR) $@

# # 生成 .o 文件的规则
# $(OBJSPATH)/%.o: $(CPPPATH)/%.cpp | $(OBJSPATH)
# 	$(CPP) -c $(CFLAGS) $< -o $@

TARGET = ./spdlog/build/libspdlog.a
BUILD_PATH = ./spdlog/build
$(TARGET): $(BUILD_PATH)
	cd $(BUILD_PATH) && cmake .. && cmake --build .
	cp $(BUILD_PATH)/libspdlog.a ../libs

$(BUILD_PATH):
	mkdir -p $(BUILD_PATH)


.PHONY : clean
clean:
	-$(RM) -r $(BUILD_PATH)
#	-$(RM) $(OBJSPATH)/*.o 