all:: libporder.so


libporder.so : place_order.c place_order.h
	gcc -shared -fPIC -o $@ $< -I/usr/include/mysql


clean: libporder.so
	rm -rf $^

install: libporder.so
	sudo cp -pr $< /usr/lib64/mysql/plugin/


