# Project: CPM
# Makefile created by Embarcadero Dev-C++ 6.3

CPP      = g++.exe
CC       = gcc.exe
OBJ      = ../bin/main.o ../bin/bdos.o ../bin/Z80.o
LINKOBJ  = ../bin/main.o ../bin/bdos.o ../bin/Z80.o
LIBS     = -static-libgcc
INCS     = 
CXXINCS  = 
BIN      = ../bin/cpm.exe
CXXFLAGS = $(CXXINCS) -D CPU_Z80_USE_LOCAL_HEADER -D CPU_Z80_STATIC -D CPU_Z80_DEPENDENCIES_H=\"Z80-support.h\" -D CPU_Z80_HIDE_ABI
CFLAGS   = $(INCS) -D CPU_Z80_USE_LOCAL_HEADER -D CPU_Z80_STATIC -D CPU_Z80_DEPENDENCIES_H=\"Z80-support.h\" -D CPU_Z80_HIDE_ABI
DEL      = del

all: $(BIN)

clean: clean-custom
	${DEL} $(OBJ) $(BIN)

$(BIN): $(OBJ)
	$(CPP) $(LINKOBJ) -o $(BIN) $(LIBS)

../bin/main.o: main.cpp
	$(CPP) -c main.cpp -o ../bin/main.o $(CXXFLAGS)

../bin/bdos.o: bdos.cpp
	$(CPP) -c bdos.cpp -o ../bin/bdos.o $(CXXFLAGS)

../bin/Z80.o: Z80.c
	$(CC) -c Z80.c -o ../bin/Z80.o $(CFLAGS)
