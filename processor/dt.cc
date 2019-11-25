#include <DataStore.h>
#include <IB_PTest.h>
#include <Executor.h>
#include <stdio.h>

#define ITER 200

int main()
{
	DataStore<Quotes,ITER>	dt("test_storage");
	dt.init("test_storage");

	Quotes* pdata = (Quotes*) dt.data(); 
	fprintf(stderr,"PTR=%p\n", pdata);

	fprintf(stderr,"Get last element pointer: F=%p, L=%p\n", pdata, dt.last());

		
	pdata->_put._bid = 22.33;
	//pdata[0]._call = 22.66;
	//pdata[1]._put = 22.34;
	//pdata[1]._call = 22.67;
	//fprintf(stderr,"D1: PUT[%g], CALL[%g]\n", pdata[0]._put, pdata[0]._call);
	//fprintf(stderr,"D2: PUT[%g], CALL[%g]\n", pdata[1]._put, pdata[1]._call);


	//Executor<Quotes,Process>   oproc(0);
	//oproc.init(pdata,ITER);
	//sleep(1);
	//oproc.run();
	//oproc.isCompleted();

	dt.dump();

	fprintf(stderr,"Attch to exsitent memory region...\n");

	DataStore<Quotes,ITER> at("test_storage");
	at.attach("test_store");

	fprintf(stderr,"ATCH PTR=0x%x\n", at.data());

	Quotes* apdata = at.data();

	fprintf(stderr,"Att value=%g\n", apdata->_put._bid);

	return 0;
}
