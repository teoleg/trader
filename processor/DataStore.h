#ifndef _DATA_STORE_H_
#define _DATA_STORE_H_
/* 
 * This file include inplementaion of DataStore based on memory map shared files.
 */

#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <memory.h>

#include <string>
#include <iostream>
#include <iomanip>
#include <sstream>

template <class OBJECT, size_t SIZE>
class DataStore
{
	int 		_fd;
	std::string	_fname;
	std::string	_store_name;
	size_t		_n_elements;
	size_t		_total_size;
	void*		_ds_addr;

public:
	DataStore(std::string store_name="def")
		:_store_name(store_name),
		 _n_elements(SIZE)
	{}
	~DataStore()
	{}

	size_t n_elements() const { return this->_n_elements; }
	size_t obj_size() const { return sizeof(OBJECT); } 

	bool init(std::string store_name="def")
	{
		_store_name = store_name;
		// build datastore filename
		char cwd[1024];
		if(!getcwd(cwd,sizeof(cwd)))
		{
			std::cerr << "Failed to get the current directory, errno=" << errno << std::endl;
			return false;
		}
		std::cerr << "Current working dir: " << cwd << std::endl;

		_fname += cwd;
		_fname += "/.GDataStore";
		_fname += "." + _store_name;;

		std::cerr << "DataStore will be located at: " << _fname << std::endl;

		_fd = open(_fname.c_str(), O_CREAT | O_RDWR);
		if(!_fd)
		{
			std::cerr << "Failed to open datastore file" << std::endl;
			return false;
		}
		//set good permissions
		std::cerr << "init: Opened filed =" << _fd << std::endl;
		if(chmod(_fname.c_str(),00644) < 0)
		{
			std::cerr << "Failed to set permissions, errno=" << errno << std::endl;
			close(_fd);
			return false;
		}
		_total_size = sizeof(OBJECT) * this->_n_elements;
		std::cerr << "Total size of the DataStore is: " << _total_size << std::endl;

		// truncate the file to the proper size
		if(ftruncate(_fd, _total_size) < 0)
		{
			std::cerr << "Failed to truncate DataStore file, errno=" << errno << std::endl;
			close(_fd);
			return false;
		}
		
		_ds_addr = mmap(0, _total_size, PROT_READ | PROT_WRITE, MAP_SHARED, _fd, 0);
		if(_ds_addr == (void*)0xffffffff)
		{
			std::cerr << "Failed to create memory mapped datastore, errno=" << errno << std::endl;
			close(_fd);
			return false;
		}

		memset(_ds_addr, 0, _total_size); 

		std::cerr << "DataStore " << _fname << " has been created succsesfuly: " << _ds_addr << std::endl;
		return true;
	}


	bool attach(std::string store_name, bool readonly = true)
	{
		_store_name = store_name;
		char cwd[1024];
		if(!getcwd(cwd,sizeof(cwd)))
		{
			std::cerr << "Failed to get the current directory, errno=" << errno << std::endl;
			return false;
		}
		std::cerr << "Current working dir: " << cwd << std::endl;
		_fname += cwd;
		_fname += "/.GDataStore";
		_fname += "." + _store_name;
		std::cerr << "DataStore is located at: " << _fname << std::endl;
		std::cerr << "Attaching with flag=" << ((readonly) ? "O_RDONLY" : "O_RDWR") << std::endl;
		_fd = open(_fname.c_str(), (readonly) ? O_RDONLY : O_RDWR);
		if(!_fd)
		{
			std::cerr << "Failed to open datastore file" << std::endl;
			return false;
		}
		std::cerr << "Attach: Opened filed =" << _fd << std::endl;
		// get stat
		struct stat st;
		if(fstat(_fd, &st) == -1)
		{
			std::cerr << "Failed to get file stat, errno = " << errno << std::endl;
			return false;
		}
		std::cerr << "Attach: file size is set to: " << st.st_size << std::endl;

		_ds_addr = mmap(0, st.st_size ,  PROT_READ, MAP_SHARED,  _fd, 0);
		if(_ds_addr == (void*)0xffffffff)
		{
			std::cerr << "Failed to create memory mapped datastore, errno=" << errno << std::endl;
			close(_fd);
			return false;
		}
		std::cerr << "DataStore " << _fname << " attached succsesfuly: " << _ds_addr << std::endl;
		return true;
	}

	void fini()
	{
		// just cleanup object, but leave file as is for now
		close(_fd);
	}

	OBJECT* data() const { return (OBJECT*) _ds_addr; }
	OBJECT* last() const { return (OBJECT*) _ds_addr + (_n_elements - 1); } 

	template<class T>
	std::string FWC(T value)
	{
	    std::stringstream ss;
	    ss.imbue(std::locale(""));
	    ss << std::fixed << value;
	    return ss.str();
	}

	void dump()
	{
		int tot = 0;
		std::cerr << "Base pointer: " << this->data() << std::endl;
		for(size_t i=0; i < _n_elements; i++)
		{
			OBJECT* pdata = this->data();
			if(!pdata) return;

			/*
			//int n = pdata->_n_results;
			//std::cerr << "N Results: " << n << std::endl;
			for(int x=0; x<n; x++)
			{
				//std::cerr << "I=" << (int) (pdata + i)->_res[x]._i << "     J=" << (int) (pdata + i)->_res[x]._j << std::endl;
			}
			*/
			tot++;
		}
		std::cerr << "Total numer of elements processed: " << FWC(tot) << std::endl;
	}
	
};


#endif
