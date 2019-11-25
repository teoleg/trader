#ifndef _INCLUDE_TEMPLATE_BODY_
#define _INCLUDE_TEMPLATE_BODY_

#include <Executor.h>
//#include <semaphore.h>
#include <iostream>



// * GLOBAL VARS **
#ifdef _MOD_STACK_SIZE
static pthread_attr_t           _thr_attr; 	// setup global thread attributes
//const size_t STACKSIZE = 1048576 * 10; 	// test. set to 10G
const size_t STACKSIZE = 16384 * 2; 		// minimize to 2MIN allowed
#endif
static pthread_cond_t          _thr_advance[MAX_GROUPS];    // condition variable, set by writing routine
static pthread_mutex_t         _thr_mutex[MAX_GROUPS];      // mutex associated with conditional wait
static pthread_barrier_t       _thr_exec_bar[MAX_GROUPS];  // global barrier (notfier) for exec completion





template <class DATA, class WORKER> 
bool Executor<DATA,WORKER>::init(DATA* pdata, size_t n_elements, int gr)
{
	if(!pdata) return false;
	if(!n_elements) return false;
	_group = gr;

	std::cerr << "GROUP is set to : " << _group << " by caller" << std::endl;

	ProcEntry* idp;

	this->_n_elements = n_elements;

	// Initialize conditional variable to control worker thread execution
	if(pthread_cond_init(&_thr_advance[_group],NULL) != 0) 
		return false;

	// Initilialize mutex associated with conditional variable (advance)
	if(pthread_mutex_init(&_thr_mutex[_group],NULL) != 0) 
	{ 
		pthread_cond_destroy(&_thr_advance[_group]); 
		return false; 
	}

#ifdef _MOD_STACK_SIZE
	// Initialize global thread attributes
	if(pthread_attr_init(&_thr_attr) != 0)
	{
		pthread_cond_destroy(&_thr_advance);
		return false;
	}

	// check the current stacksize
	size_t cst_sz = 0;
	pthread_attr_getstacksize(&_thr_attr,&cst_sz);

	std::cerr << "Current stack size = [" << cst_sz << "]" << std::endl;

	if(pthread_attr_setstacksize(&_thr_attr, STACKSIZE) != 0)
	{
		std::cerr << "Failed to update stack size to 2M" << std::endl;
		return false;
	}

	size_t ncst_sz = 0;
	pthread_attr_getstacksize(&_thr_attr,&ncst_sz);
	std::cerr << "Current stack size = [" << ncst_sz << "]" << std::endl;
#endif
	for(size_t i=0; i < n_elements ; i++)
	{
		idp = new (std::nothrow) ProcEntry();
		if(!idp)
		{
			pthread_cond_destroy(&_thr_advance[_group]);
			pthread_mutex_destroy(&_thr_mutex[_group]);
			return false;
		}
		idp->_data = (pdata) + i;
		if(!idp->_data)
		{
			pthread_cond_destroy(&_thr_advance[_group]);
			pthread_mutex_destroy(&_thr_mutex[_group]);
			delete idp;
			return false;
		}
		idp->_process = new (std::nothrow) WORKER();
		if(!idp->_process)
		{
			pthread_cond_destroy(&_thr_advance[_group]);
			pthread_mutex_destroy(&_thr_mutex[_group]);
			delete idp->_data;
			delete idp;
			return false;
		}

		idp->_group = _group;

		// kick off all the execution threads now... 
		// all threads will be wating on cond_wait and will be kicked to run by exec() call from outside..  
		if(pthread_create(&idp->_thr_id,
#ifdef _MOD_STACK_SIZE
			&_thr_attr, 
#else
			NULL,
#endif
			(void*(*)(void*))starter, idp) < 0)
		{
			std::cerr << "Failed to create a new thread" << std::endl;
			pthread_cond_destroy(&_thr_advance[_group]);
			pthread_mutex_destroy(&_thr_mutex[_group]);
			delete idp->_data;
			delete idp->_process;
			delete idp;
			return false;
		}
		//std::cerr << "A new thread created, t=" << i+1 << std::endl;
		_dataset.push_back(idp);
	}
	//std::cerr << "dataset allocation for #" << _dataset.size() << " entries completed" << std::endl;
	return true;
}

template <class DATA, class WORKER> 
bool Executor<DATA,WORKER>::fini()
{
	std::cerr << __FUNCTION__ << std::endl;
	for(size_t i=0; i < _dataset.size(); i++)
	{
		if(!_dataset[i]) continue;
		if(!_dataset[i]->_process) continue;
		if(!_dataset[i]->_thr_id) continue;
		delete _dataset[i]->_process;
		pthread_join(_dataset[i]->_thr_id, NULL);
		delete _dataset[i];
	}
	return true;
}

template <class DATA,class WORKER> 
bool Executor<DATA,WORKER>::run()
{
	// Initilize global barrier object and send broadcast signal to all worker thread to start processing
	// Upon completions of execution worker will wait on barrier till all the workers complete the job.. 
	std::cerr << "In run() group is set to : " << _group << std::endl;
	if(!this->_n_elements) 
		{ std::cerr << "Invalid state to call exec. need to call init() first" << std::endl; return false; }
	if(pthread_barrier_init(&_thr_exec_bar[_group], NULL, this->_n_elements + 1) != 0)
	{ 	
		std::cerr << "Init barrier failed" << std::endl; 
		return false; 
	}
	pthread_cond_broadcast(&_thr_advance[_group]);
	pthread_mutex_unlock(&_thr_mutex[_group]);
	return true;
}

template <class DATA,class WORKER> 
bool Executor<DATA,WORKER>::isCompleted()
{
	std::cerr << "Wait for exeutor to complete..." << std::endl;
	pthread_barrier_wait(&_thr_exec_bar[_group]);
	std::cerr << "Now destroy the barrier object" << std::endl;
	pthread_barrier_destroy(&_thr_exec_bar[_group]);
	return true;
}

template <class DATA, class WORKER> 
void* Executor<DATA,WORKER>::worker(void* arg)
{
	while(true) // endless loop. TODO replace with another cond var when main is ready.. ?? lazy... 	
	{
		ProcEntry *pdata = (ProcEntry*) arg;
		if(!pdata) break; 

		pthread_mutex_lock(&_thr_mutex[pdata->_group]);
		pthread_cond_wait(&_thr_advance[pdata->_group],&_thr_mutex[pdata->_group]); // wait for data to be ready

		// processing.. 
		if(!pdata->_process) break;
		if(!pdata->_data) break;
		if(!pdata->_process->execute(pdata->_data)) break;

		pthread_mutex_unlock(&_thr_mutex[pdata->_group]);
		pthread_barrier_wait(&_thr_exec_bar[pdata->_group]);
	}
	return 0;
}		

#endif
