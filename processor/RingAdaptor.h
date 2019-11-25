#ifndef _RING_ADAPTOR_H_
#define _RING_ADAPTOR_H_

#include <DataStore.h>
#include <vector>
#include <stdlib.h>
#include <iostream>
#include <stdio.h>

template <class DATASTORE,size_t SIZE>
class RingAdaptor
{
	std::vector<DATASTORE*>	_ring;
	size_t			_n_elements;
	DATASTORE*		_current;
	int			_cur_idx;

public:
	RingAdaptor()
	:_n_elements(SIZE),
	 _cur_idx(0)
	{
		_ring.resize(SIZE);
	}

	~RingAdaptor()
	{}

	bool init(const char* name)
	{
		for(size_t n = 0; n < this->_n_elements; n++)
		{
			std::string	ds_name_base = "RING";
			char ds_name[512];
			sprintf(ds_name,"RING_%s_%d",name,n);

			DATASTORE* pds = new (std::nothrow) DATASTORE(ds_name);
			if(!pds)
			{
				std::cerr << "Failed to allocate a new Ring Datastore: " << ds_name << std::endl;
				return false;
			}
			pds->init(ds_name);
			_ring[n] = pds;
		}

		_current = _ring[0];
		_cur_idx = 0;
		return true;
	}

	DATASTORE* current() { return _current; }

	DATASTORE* last() { return _ring[SIZE-1]; }

	bool advance() 
	{
		std::cerr << "CURR: " << _current << " != " << _ring[_n_elements-1] << std::endl;
		if((_current) != _ring[_n_elements-1])
		{
			_current = _ring[++_cur_idx];
			return true;
		}
		else if(_current+1 == _ring[SIZE-1])
		{
			_current = _ring[SIZE-1];
			_cur_idx = SIZE;
			return true;
		}
		else
		{
			_current = _ring[0];
			_cur_idx = 0;
			return true;
		}
		return false;
	}

	void rewind()
	{
		_current = _ring[0];
		_cur_idx = 0;
	}

};



#endif
