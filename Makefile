# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall 

  # the build target executable:
TARGET = hw1

all: $(TARGET)

$(TARGET): hw1.cpp
	$(CC) $(CFLAGS) hw1.cpp -o $(TARGET)

clean:
	$(RM) $(TARGET)