TWSDIR=../iblibs
TW_CLIENT=$(TWSDIR)/PosixSocketClient
TW_SHARED=$(TWSDIR)/Shared
CFLAGS=-DIB_USE_STD_STRING -DIB_USE_STD_STRIN2G -Wall -Wno-switch -fPIC
all: ../libs/libib.so

EPosixClientSocket.o : $(TWSDIR)/PosixSocketClient/EPosixClientSocket.cpp
	g++ $(CFLAGS)  -I. -I$(TW_CLIENT) -I$(TW_SHARED) -o $@ -c $<

EClientSocketBase.o : $(TWSDIR)/PosixSocketClient/EClientSocketBase.cpp
	g++ $(CFLAGS)  -I. -I$(TW_CLIENT) -I$(TW_SHARED) -o $@ -c $<

../libs/libib.so : EClientSocketBase.o EPosixClientSocket.o 
	g++ $(CFLAGS) -shared -o $@ EPosixClientSocket.o EClientSocketBase.o -I. -I$(TW_CLIENT) -I$(TW_SHARED)  -pthread

clean:
	rm -rf ../libs/libib.so EPosixClientSocket.o EClientSocketBase.o
