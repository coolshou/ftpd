CROSS_TOOLS?=${CROSS_TOOL}
CC=$(CROSS_TOOLS)gcc
CFLAGS=-c -Wall
LDFLAGS=-lm

SOURCES=server.c handles.c
OBJECTS=$(SOURCES:.cpp=.o)
EXECUTABLE=ftpd

$(EXECUTABLE): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.cpp.o:
	$(CC) $(CFLAGS) $< -o $@

clean: 
	rm -rf *.o $(EXECUTABLE)
