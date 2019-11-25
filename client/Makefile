ACE_ROOT=/root/trading/ACE/ACE_wrappers
MYSQLINC=/usr/include/mysql
TWSDIR=../iblibs
TW_CLIENT=$(TWSDIR)/PosixSocketClient
TW_SHARED=$(TWSDIR)/Shared
IBPROC=../processor
LIBS=../libs -lACE
CFLAGS=-DIB_USE_STD_STRING -DIB_USE_STD_STRIN2G -Wall -Wno-switch -I$(MYSQLINC)
LDFLAGS+=$(shell mysql_config --libs)
TIMER_IMPL:=NOTHING
#all: dbclient cltst ticker engine spy
all: dbclient 

dbclient: ticker.cc IB_DB_Client.cc Timer.cc
	g++ -g $(CFLAGS) -o $@ $^ -D$(TIMER_IMPL) -I. -I../inc -I$(ACE_ROOT) -I$(MYSQLINC) -I$(IBPROC) -I$(TW_CLIENT) -I$(TW_SHARED) -L$(LIBS) -ldbaccess -lib -lCf -pthread -Wl,-rpath -Wl,$(LIBS) 

clean:
	rm -rf dbclient *.ii *.s *.o

install: dbclient
	cp $< ../bin 
