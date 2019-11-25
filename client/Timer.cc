
#include <Timer.h>
#include <ace/TP_Reactor.h>
#include <ace/Message_Block.h>
#include <ace/OS.h>
#include <ace/Timer_Queue_T.h>
#include "IB_Client.h"


IBC_Timer::IBC_Timer()
{
        //ACE_DEBUG((LM_DEBUG,"(%t) IBC_Timer::IBC_Timer()\n"));
}


IBC_Timer::~IBC_Timer()
{
}




int IBC_Timer::start(IB_Client* p)
{
	if(!p) return -1;
	ib_client = p;
        size_t threadStackSize[] = {65536};
        return theTimer.activate(THR_NEW_LWP | THR_JOINABLE,
                        ACE_DEFAULT_THREADS,
                        1,
                        ACE_DEFAULT_THREAD_PRIORITY,
                        -1,
                        NULL,
                        NULL,
                        NULL,
                        threadStackSize,
                        NULL);

	ACE_DEBUG((LM_DEBUG,"[%D|%t] Timer Infra started\n"));
}



int IBC_Timer::handle_timeout(const ACE_Time_Value &tv, const void *arg)
{
	long *qid;
	if(!arg) return -1;

	static int f = 0; if(!f) { f = 1; (void) mysql_thread_init(); }

	qid = ((long*)arg);
	//issue orderCancel from here.. 
	ib_client->cancelOrder((long*)qid);
        ACE_DEBUG((LM_DEBUG,"[%D|%t] Timer for QID: %d\n", *qid));
        return 0;
}


long IBC_Timer::start_timer(ACE_Time_Value delay, void* arg, bool periodic)
{
        ACE_Time_Value interval = (periodic) ? delay : ACE_Time_Value::zero;
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Start Timer for QID = [%d]\n", *((long*)arg)));
        return  theTimer.schedule(this, arg, ACE_OS::gettimeofday() + delay, interval);
}



int IBC_Timer::stop_timer(long tid, const void** arg)
{
        return theTimer.cancel(tid,arg);
}
