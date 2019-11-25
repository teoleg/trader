#include <fstream>
#include <TConnector.h>
#include <poll.h>
#include <Config.h>
#include <signal.h>
#include <unistd.h>
#include <ace/Log_Msg.h>

using namespace std;

// global for testing.. should be member of main processing class..
Config configuration;
ofstream *LoggerStream;

void init_logfile(char logicalName[],int cid)
{
        char filename[80];
	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char stime[32];
        sprintf(stime,"%d%02d%02d_%02d:%02d:%02d", 
		tm.tm_year + 1900, 
		tm.tm_mon + 1, 
		tm.tm_mday, 
		tm.tm_hour, 
		tm.tm_min, 
		tm.tm_sec);

        sprintf(filename, "./logs/%s.log.%d.%s",logicalName, cid, stime);

        // LoggerStream = new ofstream(filename, ios::out | ios::app);
        LoggerStream = new ofstream(filename, ios::out | ios::trunc);

        if ((!LoggerStream) || LoggerStream->bad())   // Check for errors.
                return;
        ACE_LOG_MSG->clr_flags (ACE_Log_Msg::STDERR); // clear stderr
        ACE_LOG_MSG->set_flags (ACE_Log_Msg::OSTREAM);
	
	//ulong pmask = ACE_LOG_MSG->priority_mask();
	ACE_LOG_MSG->priority_mask(LM_DEBUG|LM_ERROR,ACE_Log_Msg::PROCESS);
        ACE_LOG_MSG->msg_ostream (LoggerStream);  // Set the ostream.
}


int main(int argc, char** argv)
{
	//u_long pmask = ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS);
        //ACE_CLR_BITS(pmask,LM_INFO|LM_WARNING|LM_STARTUP|LM_TRACE|LM_DEBUG);
        //ACE_SET_BITS(pmask,LM_ERROR /* | LM_STARTUP */);
        //ACE_LOG_MSG->priority_mask(pmask,ACE_Log_Msg::PROCESS);

	//(void) init_logfile(argv[0], 1);

	char addr[64];
	if(argc < 2)
	{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] Use default. to change IP exmaple: ./dbclient <IP> \n"));
		strcpy(addr,"127.0.0.1");
		//return -1;
	}
	else 
	{
		strcpy(addr,argv[1]);
	}

	ACE_DEBUG((LM_DEBUG,"[%D|%t] (%s) is going to connect to [%s]\n", argv[0], addr));

	if(!configuration.init("./IBC.ini"))
		return -1;

	
	//configuration.dump();
	
	TConnector<IB_Client> connector(configuration,1,LoggerStream);
	configuration.close();
	
	for(;;)
	{
		if(!connector.init(addr,4001))
			return -1;
		poll(0,0,-1);
	}

	return 0;
}
