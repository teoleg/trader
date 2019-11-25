#include <RingAdaptor.h>
#include <IB_PTest.h>

typedef DataStore<Quotes,20> Store;

int main()
{

	RingAdaptor<Store,5>	ra;
	ra.init("TEST");


	Store* pc = ra.current();
	fprintf(stderr,"current=%p, obj=%p\n",pc,pc->data());

	Store* lst = ra.last();
	fprintf(stderr,"last=%p, obj=%p\n",lst,lst->data());

	{
	ra.advance();
	Store* pc2 = ra.current();
	fprintf(stderr,"new current=%p, obj=%p\n",pc2,pc2->data());
	}

	{
	ra.advance();
	Store* pc2 = ra.current();
	fprintf(stderr,"new current=%p, obj=%p\n",pc2,pc2->data());
	}

	fprintf(stderr,"===============================\n\n");

	for(int i=0; i < 10; i++)
	{
		Store* tmp = ra.current();
		fprintf(stderr,"Current=%p, obj=%p\n", tmp, tmp->data());
		ra.advance();
		tmp = ra.current();
		fprintf(stderr,"nxt=%p, obj=%p\n\n\n", tmp, tmp->data());
	}

	ra.rewind();
	Store* rw = ra.current();
	fprintf(stderr,"rewind current=%p, obj=%p\n",rw,rw->data());

}
