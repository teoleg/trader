#ifndef _PROCESS_H_
#define _PROCESS_H_

#include <stdio.h>

struct Quotes
{
	// Generic option price data
	struct prices 
	{
		double _bid;
		double _ask;
		double _last;
	};

	// all about PUT
	prices	_put;

	// all about CALL
	prices	_call;

	// correlation ID... need to have strike price as well
	double  _strike;
	int	_call_tid;
	int 	_put_tid;

	char	_state;
};


const char CALL_BID_MASK   = 0x1;
const char PUT_BID_MASK    = 0x2;
const char CALL_ASK_MASK   = 0x4;
const char PUT_ASK_MASK    = 0x8;
const char CALL_LAST_MASK  = 0x10;
const char PUT_LAST_MASK   = 0x20;
const char ALL_MASK      = (CALL_BID_MASK|PUT_BID_MASK|CALL_ASK_MASK|PUT_ASK_MASK|CALL_LAST_MASK|PUT_LAST_MASK);

#define SET_CALL_BID(VAL)   VAL|=CALL_BID_MASK
#define SET_CALL_ASK(VAL)   VAL|=CALL_ASK_MASK
#define SET_CALL_LAST(VAL)  VAL|=CALL_LAST_MASK
#define SET_PUT_BID(VAL)    VAL|=PUT_BID_MASK
#define SET_PUT_ASK(VAL)    VAL|=PUT_ASK_MASK
#define SET_PUT_LAST(VAL)   VAL|=PUT_LAST_MASK

#define UNSET_CALL_BID(VAL)  VAL&=~CALL_BID_MASK
#define UNSET_CALL_ASK(VAL)  VAL&=~CALL_ASK_MASK
#define UNSET_CALL_LAST(VAL) VAL&=~CALL_LAST_MASK
#define UNSET_PUT_BID(VAL)   VAL&=~PUT_BID_MASK
#define UNSET_PUT_ASK(VAL)   VAL&=~PUT_ASK_MASK
#define UNSET_PUT_LAST(VAL)  VAL&=~PUT_LAST_MASK

template <char MASK> bool is_set(char value) { return (value & MASK) ? true : false; }
template <char MASK> bool is_equal(char value) { return (value == MASK) ? true : false; }

#define IS_CALL_BID_SET(VAL) is_set<CALL_BID_MASK>(VAL)
#define IS_CALL_ASK_SET(VAL) is_set<CALL_ASK_MASK>(VAL)
#define IS_CALL_LAST_SET(VAL)is_set<CALL_LAST_MASK>(VAL)
#define IS_PUT_BID_SET(VAL)  is_set<PUT_BID_MASK>(VAL)
#define IS_PUT_ASK_SET(VAL)  is_set<PUT_ASK_MASK>(VAL)
#define IS_PUT_LAST_SET(VAL) is_set<PUT_LAST_MASK>(VAL)
#define IS_ALL_SET(VAL)      is_equal<ALL_MASK>(VAL)

#define IS_CALL_SET(VAL)     (is_set<CALL_BID_MASK>(VAL) && is_set<CALL_ASK_MASK>(VAL))
#define IS_PUT_SET(VAL)	     (is_set<PUT_BID_MASK>(VAL) && is_set<PUT_ASK_MASK>(VAL))

class Process
{
	const int N, K;
	size_t tot;

public:
	Process():
		N(1),
		K(1),
		tot(0)
	{ }

	bool execute(Quotes* data)
	{
		if(!data) return false;
		fprintf(stderr,"Execute Quotes for : %p\n", data);
		for(int i=0; i < N; i++)
			for(int j=0; j < K; j++)
			{
				if(distr_test(data,
					      i, 
					      j))
				{
					//execute trade with the following combinations.
					this->exec_trade(i,j);
				}
			}
		// cleanup after run
		fprintf(stderr,"Total iterations: %ld\n", tot);
		
		if(!data) 
		{ 
			fprintf(stderr,"Invalid 'data' in execution...skip\n"); 
			return true;
		}
		//data->_put._bid = 0;
		//data->_put._ask = 0;
		//data->_put._last = 0;
		//data->_call._ask = 0;
		//data->_call._bid = 0;
		//data->_call._last = 0;
		//data->_state = 0;
		return true;
	}


	bool exec_trade(int i, int j)
	{
		// get order data here... 
		//ORDER->putOrder();
		return true;
	}

	int distr_test(Quotes* q, int i, int j)
	{
		fprintf(stderr,"I=%d, J=%d -> put:ask=%g, bid=%g, last=%g - call: ask=%g, bid=%g, last=%g\n",
			i, 
			j, 
			q->_put._ask,
			q->_put._bid,
			q->_put._last,
			q->_call._ask,
			q->_call._bid,
			q->_call._last
			);
					
		
		tot++;
		return 1;
	}

};


#endif
