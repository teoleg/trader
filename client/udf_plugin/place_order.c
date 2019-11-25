#include "place_order.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>


static const char FIFO[]="/tmp/.porder";
static int m_wo_fd;

long long place_order_fn(UDF_INIT* initid, 
			UDF_ARGS* args __attribute__((unused)),
                     	char* is_null __attribute__((unused)), 
			char* error __attribute__((unused))	
)
{
	struct sockaddr_un serveraddr;
	char DATA[32]="deadbeaf";

	if(args->args[0])
	{
		sprintf(DATA,"%d", *((int *)args->args[0]));
		//strcpy(DATA,args->args[0]);

		m_wo_fd = socket(AF_UNIX, SOCK_DGRAM, 0);	
		if(m_wo_fd < 0)
			return -errno;

		memset(&serveraddr, 0, sizeof(serveraddr));
		serveraddr.sun_family = AF_UNIX;
		strcpy(serveraddr.sun_path, FIFO);
	
		if(sendto(
			m_wo_fd, 
			DATA, 
			sizeof(DATA), 
			0,
			(const struct sockaddr *)&serveraddr, 
			sizeof(struct sockaddr_un)) < 0) 
		{
        		return -errno;	
    		}
		close(m_wo_fd);
	}
	return strlen(DATA);
}


my_bool place_order_fn_init(UDF_INIT *initid, UDF_ARGS *args,char *message)
{
	if(!(args->arg_count == 1)) 
	{
      		sprintf(message, "Incorrect number of arguments:%d",args->arg_count);
       		return 1;
     	}
	args->arg_type[0] = INT_RESULT;
	return 0;
}

my_bool place_order_fn_deinit(UDF_INIT *initid)
{
}


