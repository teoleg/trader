#ifndef _IB_LOGGER_H_
#define _IB_LOGGER_H_



class Logger : public ACE_Log_Msg_Callback
{
public:
    virtual void log(ACE_Log_Record &log_record)
    {
        u_long pri = log_record.type();
        switch(pri)
        {
                case LM_TRACE:
                        fprintf(stderr,"%s",(char *)log_record.msg_data());
                break;

                case LM_ERROR:
                case LM_WARNING:
                        fprintf(stderr,"%s",(char*)log_record.msg_data());
                break;

                case LM_DEBUG:
                        fprintf(stderr,"%s",(char*)log_record.msg_data());
                break;
        }

    }
};


#endif
