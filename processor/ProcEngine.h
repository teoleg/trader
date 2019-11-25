#ifndef _PROC_ENGINE_H_
#define _PROC_ENGINE_H_


#include <RingAdaptor.h>
#include <Executor.h>
#include <IB_PTest.h>

const int DSIZE=2; // number blocks in the data store
const int RSIZE=2; // number datastores
const char CTRL_PATH[] = ".local";

typedef DataStore<Quotes,DSIZE>	DSTORE;

class ProcEngine
{
	RingAdaptor<DSTORE,RSIZE>	_ring;
	Executor<Quotes,Process>	_exec[RSIZE];
	
	typedef RingAdaptor<DSTORE,RSIZE>	RING;
	typedef Executor<Quotes,Process>	PROC;

	typedef std::pair<RING,PROC>		BUNDLE;

	std::vector<BUNDLE>			_bundle;
	pthread_t			_ctrl_tid;
	int				_r_fd;
	int				_w_fd;
public:
	ProcEngine()
	{}

	~ProcEngine()
	{
		unlink(CTRL_PATH);
	}

	bool init()
	{
		std::cerr << "Input: DSIZE=" << DSIZE << ", RSIZE=" << RSIZE << std::endl;
		if(!_ring.init("PE"))
		    return false;

		// travers thru ring buffers and assign executor to each ds instance
		for(int rings = 0; rings < RSIZE; rings++)
		{
			DSTORE* dt = _ring.current();
			// each ring buffer has RSIZE pools. Need to assign inidividual Executor to each pool.. ???
			for(int ds = 0; ds < dt->n_elements(); ds++)
			{
				if(!_exec[ds].init(dt->data(),DSIZE,ds))
					return false;
				sleep(2);
			}
			_ring.advance();
		}

		// build communcation channel
		unlink(CTRL_PATH);
		if(mkfifo(CTRL_PATH,0666) < 0)
		{
			std::cerr << "Failed to alocate FIFO for ProcEng, errno=" << errno << std::endl;
			return false;
		}
		std::cerr << "FIFO created" << std::endl;

		// allocate control thread.
		if(pthread_create(&_ctrl_tid,NULL,(void*(*)(void*))start,this) < 0)
		{
			std::cerr << "Failed to build CTRL thread, errno = " << errno << std::endl;
			close(_w_fd);
			unlink(CTRL_PATH);
			return false;
		}

		_w_fd = ::open(CTRL_PATH,O_CREAT|O_WRONLY);
		if(!_w_fd)
		{
			std::cerr << "Failed to open write side, errno=" << errno << std::endl;
			return false;
		}
		std::cerr << "Writer side is opened, fd=" << _w_fd << std::endl;
		return true;
	}

	static void* start(void* arg) {
		std::cerr << "Thread starter called" << std::endl;
                return reinterpret_cast<ProcEngine*>(arg)->ctrl_job(arg);
        }


	void* ctrl_job(void* arg)
	{
		std::cerr << "Kicked CTRL thread" << std::endl;
		char message[16]; 
		std::cerr << "Wait for init to open write side.. " << std::endl;
		sleep(5);
		this->_r_fd = ::open(CTRL_PATH,O_RDONLY);
		if(!this->_r_fd)
		{
			std::cerr << "Failed to open read side, errno = " << std::endl;
			return 0;
		}
		std::cerr << "Channel is opened." << std::endl;
		while(1)
		{
			if(read(_r_fd,message,sizeof(message)) < 0)
			{
				std::cerr << "Failed to process CTRL mesage, errno = " << errno << std::endl;
				close(_r_fd);
				return 0;
			}
			std::cerr << "Received CTRL: " << message << std::endl;
			int id = atoi(message);
			_exec[id].run();
			_exec[id].isCompleted();
		}
	}

	bool process(int id)
	{
		char message[16];
		sprintf(message,"%d",id);
		if(write(_w_fd,message,sizeof(message)) < 0)
		{
			std::cerr << "Failed to notify CTRL task, errno=" << errno << std::endl;
			return false;
		}
		std::cerr << "CTRL task has been notified" << std::endl;
		return true;
	}
};




#endif
