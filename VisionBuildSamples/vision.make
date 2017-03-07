# the compiler
CC = g++

#compiler flags:
CFLAGS = -std=c++0x `pkg-config --cflags --libs opencv`
TARGET = test

all: $(TARGET)

$(TARGET): $(TARGET).cpp
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).cpp


clean:
	$(RM) (TARGET)
	$(RM) (CAMSERV)
