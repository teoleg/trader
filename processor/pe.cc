#include <ProcEngine.h>
#include <poll.h>

int main()
{
	ProcEngine	pe;
	pe.init();
	std::cerr << "main will wait for 10 sec... " << std::endl;
	sleep(10);
	std::cerr << "Call for processing...." << std::endl;
	pe.process(2);

	poll(0,0,-1);

}
