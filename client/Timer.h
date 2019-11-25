#ifndef _IB_TIMER_H_
#define _IB_TIMER_H_

#include <ace/Task.h>
#include <ace/Timer_Queue_Adapters.h>
#include <ace/Timer_Heap.h>
#include <ace/Timer_Wheel.h>
#include <ace/Log_Msg_Callback.h>

class IB_Client;

class IBC_Timer : public ACE_Event_Handler
{
public:

        IBC_Timer();
	~IBC_Timer();

        int start(IB_Client* p);
        int handle_timeout(const ACE_Time_Value &tv, const void *arg);
        long start_timer(ACE_Time_Value delay,void* arg, bool periodic=false);
        int stop_timer(long tid, const void** arg=0);

#ifdef IB_USE_TIMER_WHEEL
        typedef ACE_Thread_Timer_Queue_Adapter<ACE_Timer_Wheel> TIMERQ;
#else
        typedef ACE_Thread_Timer_Queue_Adapter<ACE_Timer_Heap> TIMERQ;
#endif


private:
        TIMERQ                	theTimer;
	IB_Client		*ib_client;
};


#endif
