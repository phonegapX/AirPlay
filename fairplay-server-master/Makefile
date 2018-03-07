CC=gcc
#CFLAGS = -m32 -g -O2 -DRUN_DAEMON
CFLAGS = -m32 -g -O2 -DDEBUG
LDFLAGS = -lpthread 

all:	
	$(CC) $(CFLAGS) fairplay-server.c fairplay.c fairplay-core.o -o fairplay-server $(LDFLAGS)
	$(CC) $(CFLAGS) airplay.c fairplay.c fairplay-core.o -o airplay $(LDFLAGS)
clean:
	rm -f fairplay-server airplay
