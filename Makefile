RM=rm -rf
MKDIR=mkdir -p

CC=gcc
INCLUDES=-I$(shell pwd)/include
CPPFLAGS=$(INCLUDES) -DMACH3

LD=gcc
LIBS=-lstdc++
LDFLAGS=$(LIBS)

SRCDIR=src
OBJDIR=obj

VPATH=$(SRCDIR) $(OBJDIR)

CPPFILES=main.cpp parser.cpp pcb-probe.cpp
EXEFILE=pcb-probe

OFILES=$(CPPFILES:%.cpp=%.o)

all: $(EXEFILE)

clean:
	$(RM) $(OBJDIR)

$(OBJDIR):
	$(MKDIR) $@

$(EXEFILE): $(OBJDIR) $(OFILES)
	$(LD) $(LDFLAGS) $(OFILES:%=$(OBJDIR)/%) -o $(EXEFILE)

.cpp.o: 
	$(CC) -c $(CPPFLAGS) $< -o $(OBJDIR)/$@
