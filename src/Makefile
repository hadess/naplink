CC	=	gcc
STRIP	=	strip
CFLAGS	=	-O2 -Wall
LDFLAGS	=	-lusb-1.0
RM	=	rm -f
CP	=	cp
CHMOD	=	chmod

INSTDIR	=	/usr/local/psx2dev/bin

all: naplink

naplink: naplink.o packet.o pl2301.o
	$(CC) $(LDFLAGS) $^ -o $@

install: naplink
	$(CP) $< $(INSTDIR)
	$(STRIP) $(INSTDIR)/$<
	$(CHMOD) +s $(INSTDIR)/$<

clean:
	$(RM) *.o

distclean:
	$(RM) naplink *.o
