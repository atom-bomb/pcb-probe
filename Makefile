RM=rm -rf
MKDIR=mkdir -p

VERSION?=$(shell git describe --always --dirty)

CC=g++
INCLUDES=-I$(shell pwd)/include
CPPFLAGS=$(INCLUDES) -DVERSION="\"$(VERSION)\"" -DDEFAULT_GCODE_TYPE=emc

LD=g++
LIBS=-lstdc++ -lm
LDFLAGS=$(LIBS)

SRCDIR=src
OBJDIR=obj

VPATH=$(SRCDIR) $(OBJDIR)

CPPFILES=$(shell cd $(SRCDIR) ; ls *.cpp)
OFILES=$(CPPFILES:%.cpp=%.o)

EXEFILE=pcb-probe

all: $(EXEFILE)

clean:
	$(RM) $(OBJDIR)

$(OBJDIR):
	$(MKDIR) $@

$(EXEFILE): $(OBJDIR) $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES:%=$(OBJDIR)/%) -o $(EXEFILE)

.cpp.o: 
	$(CC) -c $(CPPFLAGS) $< -o $(OBJDIR)/$@

.PHONY: clean all
