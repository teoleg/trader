#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#include <pthread.h>
#include <vector>

const int MAX_GROUPS = 64;

template <class DATA, class WORKER>
class Executor
{
	int			_group;

public:
	struct ProcEntry
	{
		DATA*			_data;
		WORKER* 		_process;
		pthread_t		_thr_id;
		int			_group;
	};

	std::vector<ProcEntry*> 	_dataset;
	size_t 				_n_elements;


	Executor<DATA,WORKER>(int g=0)
	:_group(g)
	{}
	~Executor() { }

	bool init(DATA* pdata, size_t n_elements, int gr=0);
	bool fini();

	bool run();
	bool isCompleted();

	static void* starter(void* arg) {
		return reinterpret_cast<Executor*>(arg)->worker(arg);
	}

	void* worker(void* arg);
};


// gcc requires template body... 
#include <Executor.cc>


#endif
