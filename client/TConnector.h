#ifndef _T_CONNECTOR_H_
#define _T_CONNECTOR_H_


#include <pthread.h>
#include <vector>
#include <iostream>
#include <IB_Client.h>
#include <poll.h>

template <class CLIENT>
class TConnector
{
	struct TSD
	{
		pthread_t	_tid;
		pthread_t	_order_tid;
		int		_cid;
		CLIENT*		_cli;
	};
	std::vector<TSD*>	_tpool;
	int			_nth;
	Config			_cf;
	ofstream 		*_logger;
public:
	TConnector(Config& cf, int nth, ofstream*& logger)
		:_nth(nth)
		,_cf(cf),
		_logger(logger)
	{
		_tpool.resize(nth);
	}

	~TConnector()
	{
		//std::cerr << "Distructor" << std::endl;
	}

	bool init(const char* host, int port)
	{

		for(size_t i = 0; i < _tpool.size(); i++)
		{
			
			_tpool[i] = (TSD*) new (std::nothrow) TSD;
			if(!_tpool[i])
			{	
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to allocate memory for pthread entry\n"));
				return false;
			}

			_tpool[i]->_cid = i+1;

			_tpool[i]->_cli = new (std::nothrow) CLIENT(_cf,_tpool[i]->_cid);
			if(!_tpool[i]->_cli)
			{
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to allocate client object\n"));
				return false;
			}

			_tpool[i]->_cli->init_data_store();

			if(!_tpool[i]->_cli->open_channel())
			{
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to create communcation channel\n"));
				return false;
			}
			sleep(5);
			ACE_DEBUG((LM_DEBUG,"[%D|%t] Channel opened. Order's processing thread is being created\n"));

			if(pthread_create(&_tpool[i]->_tid,
				NULL,
				(void*(*)(void*))TConnector::starter,
				_tpool[i]) < 0)
			{
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to create a new thread..exit\n"));
				return false;
			}
			sleep(5);
			ACE_DEBUG((LM_DEBUG,"[%D|%t] INFO: Allocated thread# %d\n",_tpool[i]->_tid));
			


			if(pthread_create(&_tpool[i]->_order_tid,
					NULL,
					(void*(*)(void*))TConnector::order_starter,
					_tpool[i]) < 0)
			{
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to create a new thread for ordering...\n"));
				return false;
			}
			sleep(5);
			ACE_DEBUG((LM_DEBUG,"[%D|%t] INFO: Ordering thread created #%d\n",_tpool[i]->_order_tid));

			bool connected = false;
			int retry=0;
			do
			{
				if(!_tpool[i]->_cli->connect(host,port))
				{
					ACE_DEBUG((LM_ERROR,"[%D|%t] Cannot establish connection with TWS.... Retrding (%d)\n",++retry));
					::poll(0,0,1000);
					return false;
				}
				else connected=true;
			}
			while(!connected);
			ACE_DEBUG((LM_DEBUG,"[%D|%t] APP is connected\n"));
		}	
		return true;
	}

	void fini()
	{
		for(size_t i=0; i<_tpool.size(); i++)
		{
			//pthread_desroy(_tpool[i]);
			
		}
	}

	static void* starter(void* arg) {
		return reinterpret_cast<TConnector*>(arg)->service(arg);
	}


	static void* order_starter(void* arg) {
		return reinterpret_cast<TConnector*>(arg)->order_service(arg);
	}

	void* order_service(void* arg)
	{
		TSD* tsd = (TSD*) arg;
		if(!tsd) { return 0; }
		if(!tsd->_cli) { return 0; }
		while(true)
		{
			tsd->_cli->read_channel();	
		}
		return NULL;
	}


	void* service(void* arg)
	{
		TSD* tsd = (TSD*) arg;
		if(!tsd) { return 0; }
		if(!tsd->_cli) { return 0; }
		while(true)
		{
			if(tsd->_cli->isConnected())
				tsd->_cli->processMessages();
		}
		return 0;
	}
};





#endif
