#ifndef __P_PROCESS_H_
#define __P_PROCESS_H_

#include <stdio.h>

struct Quotes
{
	struct prices 
	{
		double _bid;
		double _ask;
		double _last;
	};
	prices	_put;
	prices	_call;
	int	_cid;
};

class Process
{
	const int N, K;
	size_t cnt;
public:
	Process():
		N(1),
		K(1),
		cnt(0) 
	{ }

	bool execute(Quotes* data)
	{
		if(!data) return false;
		int ii = 0;
		for(int i=0; i < N; i++)
			for(int j=0; j < K; j++)
			{
				if(distr_test(data,
					      i, 
					      j))
				{
					//execute trade with the following combinations.
				}
			}
		return true;
	}

	int distr_test(Quotes* p3, int i, int j)
	{
		fprintf(stderr,"DIST: 0x%lx: PTR=%p\n",pthread_self(),p3);
		return 1;
	}
};


#endif
