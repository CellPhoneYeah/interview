CC = gcc
CPP = g++
RM = rm -f

CFLAGS = -I ./srcs -O0 -shared -g -Wall -fexceptions -fPIC -DLINUX -lpthread

SRCS = ./srcs
OBJSPATH = ./objs
TARGET = ./test

$(OBJSPATH)/%.o: $(SRCS)/%.cpp
	$(CPP) $(CFLAGS) $< -o $@

$(TARGET):$(OBJSPATH)/*.o
	$(CPP) $(CFLAGS) $(OBJSPATH)/*.o -o $(TARGET)


# BIN = ./bin
# OBJDIR = ./objs
# SRCS = ./srcs
# CPPFILES = $(wildcard $(SRCS)/*.cpp)
# $(info CPPFILES are $(CPPFILES))
# OBJFILES = $(patsubst $(SRCS)/%.cpp, $(OBJDIR)/%.o, $(CPPFILES))
# $(info OBJFILES are $(OBJFILES))

# TARGET = $(BIN)/EpollManagerTest
# $(info TARGET are $(TARGET))

# $(OBJDIR):
# 	mkdir -p ./$@

# $(BIN):
# 	mkdir -p ./$@

# $(OBJDIR)/%.o: $(SRCS)/%.cpp
# 	$(CPP) -c $(CFLAGS) $< -o $@
	
# $(OBJDIR)/%.o: $(SRCS)/%.cpp
# 	$(CPP) -c $(CFLAGS) $< -o $@

# $(TARGET): $(BIN) $(OBJDIR) $(OBJFILES)
# 	$(CPP) $(CFLAGS) $(OBJFILES) -o $(TARGET)

# $(TARGET): $(BIN) $(OBJDIR) $(OBJFILES)
# 	$(CPP) $(CFLAGS) $(OBJFILES) -o $(TARGET)

.PHONY : clean
clean:
	-$(RM) $(TARGET)
	-$(RM) $(OBJSPATH)/*.o 
	-$(RM) $(SRCS)/*.o 