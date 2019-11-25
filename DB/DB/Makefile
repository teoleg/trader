TARGETS=libdbaccess.so t
all: ${TARGETS} 

CPPFLAGS+=$(shell mysql_config --cflags) -I.
LDFLAGS+=$(shell mysql_config --libs)

libdbaccess.so : DBAccess.cc
	g++ -g --shared ${CPPFLAGS} -o $@ $^ ${LDFLAGS}
	cp $@ ../../libs

t : t.cc
	g++ -g ${CPPFLAGS} -o $@ $^ -L. -ldbaccess ${LDFLAGS}

clean:
	rm -rf ${TARGETS} *.ii *.s *.o
	
