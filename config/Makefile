TWSDIR=../iblibs
TW_CLIENT=$(TWSDIR)/PosixSocketClient
TW_SHARED=$(TWSDIR)/Shared
CFLAGS=-DIB_USE_STD_STRING -DIB_USE_STD_STRIN2G -Wall -Wno-switch -fPIC
all:  tcf ../libs/libCf.so

tcf : Config.cc tcf.cpp
	g++ -g $(CFLAGS) -I$(TW_SHARED) -I. -o $@ $^ -L../libs -lini -Wl,-rpath -Wl,../libs

../libs/libCf.so : Config.cc 
	g++ -shared -g $(CFLAGS) -I$(TW_SHARED) -I. -o $@ $^ -L../libs -lini -Wl,-rpath -Wl,../libs

clean:
	rm -rf ../libs/libCf.so tcf
