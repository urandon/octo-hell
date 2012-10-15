#
# Makefile
#
SOURCES.c= main.c parser.c
INCLUDES=parser.h
CLAGS=
SLIBS=
PROGRAM= octo-hell

OBJECTS= $(SOURCES.c:.c=.o)

.KEEP_STATE:

debug := CFLAGS= -g

all debug: $(PROGRAM)

$(PROGRAM): $(INCLUDES) $(OBJECTS)
	$(LINK.c) -o $@ $(OBJECTS) $(SLIBS)
	
clean:
	rm -f $(PROGRAM) $(OBJECTS)
