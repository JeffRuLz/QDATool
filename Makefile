CC = g++

INCLUDE_PATHS = -IC:\MinGW\mingw32\include
LIBRARY_PATHS = -LC:\MinGW\mingw32\lib

CFLAGS := -std=c++17

LIBS = -lm -lmingw32

INCLUDES = 

OBJS := $(wildcard ./*.cpp)

TARGET = qdatool
#---------------------------------------------

.PHONY: $(TARGET) clean

all: $(TARGET)
	
$(TARGET):
	$(CC) $(OBJS) $(INCLUDE_PATHS) -I$(INCLUDES) $(LIBRARY_PATHS) $(CFLAGS) $(LIBS) -o $(TARGET)

clean:
	@rm -fr $(TARGET)