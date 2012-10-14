# (!c) petter wahlman, aka badeip
#
SRCDIR	= ./src
OBJDIR   = ./obj
BINDIR	= ./bin
INCLUDE	= ./include

TVIXFW	= $(BINDIR)/tvixfw

LDFLAGS = -lz
CFLAGS = -I $(INCLUDE) -g -Wall -Wunused

VPATH		= $(SRCDIR)

OBJECTS = \
	$(OBJDIR)/tvixfw.o \
	$(OBJDIR)/crc.o \
	$(OBJDIR)/crypt.o \
	$(OBJDIR)/zlib.o \

$(OBJDIR)/%.o:	%.c
	$(CC) -c $< $(CFLAGS) -o $@

.PHONY:
all:	make_dirs $(TVIXFW)

$(TVIXFW): $(OBJECTS)
	$(CC) -o $@ $^ $(LDFLAGS) 

.PHONY:
clean:
	@rm -rvf \
		$(TVIXFW) \
		$(OBJDIR)/*.o  \

.PHONY:
make_dirs:
	@mkdir -p $(OBJDIR) $(BINDIR)
