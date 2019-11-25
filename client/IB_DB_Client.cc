#include <IB_Client.h>
#include <pthread.h>

#include <sstream>
#include <iostream>

// temp
#include <assert.h>

#include "EPosixClientSocket.h"
#include "EPosixClientSocketPlatform.h"

#include "Contract.h"
#include "Order.h"
#include "Execution.h"

#include <time.h>

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <poll.h>

#define QENTRY(TID) (m_quotes[m_qmapper[TID].theQ])
#define MYSQL_THREAD_REGISTER do { \
		static bool f = false; \
		if(!f) { f = true; (void) mysql_thread_init();  ACE_DEBUG((LM_DEBUG,"[%D|%t] Register thread# %t once\n")); } \
		}while(0)

IB_Client::IB_Client(Config& cf,int id)
	:m_id(1),
	 m_state(STATE_UNDEF),
	 m_conf(cf),
	 m_clientId(id),
	 m_pClient(new EPosixClientSocket(this))
{
	m_id=m_clientId;
	memset(m_qmapper,0,sizeof(m_qmapper));
	ACE_DEBUG((LM_ERROR,"[%D|%t] m_id for client# %d is %d\n", m_clientId, m_id));

	// init spilock here
	pthread_spin_init(&m_db_spin, 0);

}


IB_Client::~IB_Client()
{
}

bool IB_Client::init_data_store()
{
	if(!m_dba.init())
        {
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to initialize connection to the DB\n" ));
                return false;
        }

	if(!m_db_orders.init())
        {
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to initialize connection to the DB\n"));
                return false;
        }

	if(!m_db_sell.init())
        {
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to initialize connection to the DB\n"));
                return false;
        }

	if(m_timer.start(this) < 0)
	{
		ACE_DEBUG((LM_DEBUG, "[%D|%t] Failed to initialize Timer infra\n"));
		return false;
	}
	sleep(5);
	ACE_DEBUG((LM_DEBUG, "[%D|%t] Timer INFRA is started\n"));
	

	if(!m_conf.getByClient(m_quotes,m_clientId))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] There are no quotes assotiated with client #%d. Check ini file\n", m_clientId));
		return false;
	}

	// just dump it for now...
	for(size_t i=0; i < m_quotes.size(); i++)
	{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] DUMP accepted Quotes for #%d\n", m_clientId));
		m_quotes[i].dump();
	}
	ACE_DEBUG((LM_DEBUG,"[%D|%t] --------- END-------------\n"));

	for(size_t q=0 ; q < m_quotes.size() ; q++)
	{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] Q# %d, ASSIGNED ID: CALL: %d, PUT: %d\n", q, m_quotes[q]._callID, m_quotes[q]._putID));
		m_qmapper[m_quotes[q]._callID].theQ = q;
		m_qmapper[m_quotes[q]._callID].type = CALL;

		m_qmapper[m_quotes[q]._putID].theQ = q;
		m_qmapper[m_quotes[q]._putID].type = PUT;

	}

	m_timer_id_map.resize(65000);
	return true;
}


bool IB_Client::open_channel()
{
	struct sockaddr_un serveraddr;
	unlink(PORD);
	m_wr_fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(m_wr_fd < 0)
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to open order FD\n"));
		return false;	
	}

	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sun_family = AF_UNIX;
	strcpy(serveraddr.sun_path, PORD);

	int rc = bind(m_wr_fd, (struct sockaddr *)&serveraddr, SUN_LEN(&serveraddr));
	if(rc < 0)
	{	
		ACE_DEBUG((LM_ERROR,"[%D|%t] bind failed: errno: %d\n", errno));
		return false;
	}
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Writer side is opened, fd=%d\n", m_wr_fd));
        return true;
}


bool IB_Client::read_channel()
{	
	int rc = 0;
	char buf[128];
	if(m_wr_fd)
	{
		if((rc = read(m_wr_fd,buf,128)) < 0)
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to read from the socket\n"));
			return false;	
		}
		std::cerr << "triggered: [" << buf << "]" << std::endl;
		this->placeOrder(buf);
	}
	return true;
}


void* IB_Client::executor(void* arg)
{
	return NULL;
}

bool IB_Client::connect(const char *host, unsigned int port)
{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Connecting to server: %s:%d, SID=%d\n", host, port, m_clientId));
	if(!m_pClient->eConnect(host, port, m_clientId))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to connect to the TWS... \n"));
		return false;
	}
	return true;
}


void IB_Client::disconnect() const
{
	m_pClient->eDisconnect();
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Disconnected\n"));
}

bool IB_Client::isConnected() const
{
        return m_pClient->isConnected();
}


void IB_Client::reqRealTimeBars(void)
{
	Config::SData sd;
	if(!m_conf.getFirst(sd))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to get message from config\n"));
		return;
	}
	sd.dump();
	IBString wts;
	wts = sd._wts; 
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Send Request for Real Time Bars, wts=%s\n",wts.c_str()));
	m_state = STATE_RTB_SENT;
	m_pClient->reqRealTimeBars(m_id, sd._contract, sd._bar_size, wts, 1);
	
}


void IB_Client::reqHistData(void)
{
	Config::SData sd;
	if(!m_conf.getFirst(sd))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to get message from config file:f=%s\n",__FUNCTION__));
		return;
	}
	sd.dump();
        ACE_DEBUG((LM_DEBUG,"[%D|%t] Send Request Historical Data\n"));
        m_state = STATE_RHD_SENT;
        m_pClient->reqHistoricalData(m_id, 
				sd._contract, 
				sd._endDateTime, 
				sd._durationStr, 
				sd._barSizeSetting, 
				sd._wts, 
				sd._useRTH , 
				sd._formatDate);
}

void IB_Client::reqMarketData(void)
{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Send Request Market Data...\n"));
	m_state = STATE_RMD_SENT;
	for(size_t q=0 ; q < m_quotes.size() ; q++)
	{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] Q# %d ASSIGNED ID: SYM(%s):%.2f call:%d, put:%d\n",
				q, 
				m_quotes[q]._contract.symbol.c_str(),
				m_quotes[q]._contract.strike,
				m_quotes[q]._callID,
				m_quotes[q]._putID));

		if(m_quotes[q]._contract.secType == "OPT")
		{
			m_quotes[q]._contract.right = "CALL";
				(void) m_pClient->reqMktData(m_quotes[q]._callID, m_quotes[q]._contract, m_quotes[q]._tickType, false);

			m_quotes[q]._contract.right = "PUT";
				(void) m_pClient->reqMktData(m_quotes[q]._putID, m_quotes[q]._contract, m_quotes[q]._tickType, false);
		}
		else 
		{
			(void) m_pClient->reqMktData(m_quotes[q]._callID, m_quotes[q]._contract, m_quotes[q]._tickType, false);
		}
	}

 
}


void IB_Client::reqOpenOrders()
{
	 //m_pClient->reqOpenOrders();
}


void IB_Client::processMessages()
{
	//std::cerr << "Calling processMessages for " << pthread_self() << " client ID=" << m_id << std::endl;
	fd_set readSet, writeSet, errorSet;
	struct timeval tval;
	tval.tv_usec = 0;
	tval.tv_sec = 0;

	switch(m_state)
	{
		case STATE_SEND_RMD:
			this->reqMarketData();
		break;

		case STATE_SEND_RTB:
			this->reqRealTimeBars();
		break;
		case STATE_SEND_RHD:
			this->reqHistData();
		break;
	}

	if( m_pClient->fd() >= 0 ) {
		FD_ZERO( &readSet);
		errorSet = writeSet = readSet;
		FD_SET( m_pClient->fd(), &readSet);
		if( !m_pClient->isOutBufferEmpty())
			FD_SET( m_pClient->fd(), &writeSet);

		FD_CLR( m_pClient->fd(), &errorSet);
		int ret = select( m_pClient->fd() + 1, &readSet, &writeSet, &errorSet, 0);
		
		if( ret == 0) {
			return;
		}
		if( ret < 0) {
			disconnect();
			ACE_DEBUG((LM_ERROR,"[%D|%t] Disconnect\n"));
			return;
		}

		if( m_pClient->fd() < 0)
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Client FD -1\n"));
			return;
		}
	
		if( FD_ISSET( m_pClient->fd(), &errorSet)) 
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Error on socket\n"));
			m_pClient->onError();
		}

		if( m_pClient->fd() < 0)
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Invalid FD after error set\n"));
			return;
		}

		if( FD_ISSET( m_pClient->fd(), &writeSet))
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Call on Send\n"));
			m_pClient->onSend();
		}

		if( m_pClient->fd() < 0)
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Invalid FD after write set\n"));
			return;
		}		

		if( FD_ISSET( m_pClient->fd(), &readSet)) 
		{
			m_pClient->onReceive();
		}
	}
}

void IB_Client::orderStatus( OrderId orderId, const IBString &status, int filled,
           int remaining, double avgFillPrice, int permId, int parentId,
           double lastFillPrice, int clientId, const IBString& whyHeld)

{

}

void IB_Client::historicalData(TickerId reqId, const IBString& date, double open, double high,
                              double low, double close, int volume, int barCount, double WAP, int hasGaps) 

{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] CB: historicalData() - received\n"));
	m_state = STATE_RHD_RCVD;
	ACE_DEBUG((LM_DEBUG,"[%D|%t] RID=%d, date=%s, open=%f,high=%f,low=%f,close=%f,vol=%d,barc=%d,WAP=%f,hasGap=%d\n",
			reqId, date.c_str(), open, high, low, close, volume, barCount, WAP, hasGaps));
}

void IB_Client::realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                           long volume, double wap, int count) 
{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] CB: RealTimeBar - received\n"));
	ACE_DEBUG((LM_DEBUG,"[%D|%t] CB: TID=%d,time=%d,open=%10f,high=%10f,low=%10f,close=%10f,vol=%d,wap=%10g,count=%d\n",
				reqId, time,open,high,low,close,volume,wap,count));
}

void IB_Client::nextValidId( OrderId orderId)
{

	MYSQL_THREAD_REGISTER;

	ACE_DEBUG((LM_DEBUG,"[%D|%t] nextValidId - invoked, ID=%d\n", (int)orderId));

	m_next_id = orderId;

	Config::SData sd;
	if(!m_conf.getFirst(sd))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Can't get the type from Config\n"));
		return;
	}

	if(!sd._command.compare("REQUEST_HISTORICAL_DATA"))
	{
		m_state = STATE_SEND_RHD;
	}
	else if(!sd._command.compare("REQUEST_MARKET_DATA"))
	{
		m_state = STATE_SEND_RMD;
	}
	else if(!sd._command.compare("REQUEST_REALTIME_BARS"))
	{
		m_state = STATE_SEND_RTB;
	}
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Set message type to:[%d](%s)\n", m_state, sd._command.c_str()));
}

void IB_Client::currentTime( long time)
{
}

void IB_Client::error(const int id, const int errorCode, const IBString errorString)
{
	switch(errorCode)
	{
		default:
			ACE_DEBUG((LM_ERROR,"[%D|%t] Error id=%d, errorCode=%d, \n-------------------- message from TWS --------------------\n\t%s\n----------------------------------------------------------\n", 
					id, 
					errorCode, 
					errorString.c_str()));
			if(errorCode == 399)
			{ 	
				//local_cleanup(id);
				//m_pClient->cancelOrder(id);
			}
		break;
		case 2104:
		case 2119:
		case 2107:
		break;
	}
	if( id == -1 && errorCode == 1100)
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Call Disconnect on a client socket\n"));
		disconnect();
	}
}


/* Supported types 
enum eTikerType
	1 = bid
	2 = ask
	4 = last
	6 = high
	7 = low
	9 = close
*/

void IB_Client::tickPrice( TickerId tickerId, TickType field, double price, int canAutoExecute) 
{
}

void IB_Client::tickPriceEx( TickerId tickerId, TickType field, double price, int canAutoExecute, TickType szFileld, int size) 
{
	// just for debugging 
	int TType = m_qmapper[tickerId].type;
	struct { int id; char name[8]; } X[] = 
	{
		{ 0, "EMPTY" },
		{ 1, "BID" },
		{ 2, "ASK" },
		{ 3, "OTHER" },
		{ 4, "LAST"},
		{ 5, "OTHER" },
		{ 6, "HIGH" },
		{ 7, "LOW" },
		{ 8, "OTHER" },
		{ 9, "CLOSE" },
		{ 10, "END" }
	};
	if(field == ASK || field == BID /**/|| field == LAST/**/)
	{
		if(QENTRY(tickerId)._contract.secType == "OPT")
		{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] RCVD[OPT]: [%-4s:%-5.2f,%-4s,%-4s] price:[%-4.2f], for ticker=%d where Quote index: %d\n", 
			QENTRY(tickerId)._contract.symbol.c_str(),
			QENTRY(tickerId)._contract.strike,
			(TType == PUT) ? "PUT" : "CALL", 
			X[field].name,
			price, 
			tickerId, 
			m_qmapper[tickerId].theQ)); 
		}
		else
		{
		ACE_DEBUG((LM_DEBUG,"[%D|%t] RCVD[STOCK]: [%-4s:%-4s] price:[%-4.2f], for ticker=%d where Quote index: %d\n", 
			QENTRY(tickerId)._contract.symbol.c_str(),
			X[field].name,
			price, 
			tickerId, 
			m_qmapper[tickerId].theQ)); 
		}
	}

	// get a date.. :-)
	time_t t = time(NULL);
        struct tm tm = *localtime(&t);
	char stime[32];
        sprintf(stime,"%d-%02d-%02d %02d:%02d:%02d", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	if(QENTRY(tickerId)._contract.secType == "OPT") {

		//The new logic here. We need to count each event (LAST trade 1ph) and issue cancellation when in the defined interval no trade is made.
		// Interval and count of events will be provided by Eugene via orders_queue (cancelInterval + number of events). 
		// m_q or mapper will be modifed to include a new counters... per quote. 
		//
		// B?---- 
		//  - how to corelete count event with open orders... Need to map OrderId with Quote
		//  - What is an identification for particular quote now. 
		//  - How many Orders can be triggered by a single quote ???  
		//

		//ACE_DEBUG((LM_DEBUG,"[%D] OPT CHECK CID=%d\n", QENTRY(tickerId)._cancel_tid));
		//
		// need to query pending queue here for all the orders for particular security... 
		// Then validate nEvents for each and cancel expired... make sense :-) ???
		//
		// m_dba.get_all_recoreds_from_pending(lookupkey,lookupvalue,symbol,output_list);
		//
		//
		// for(output_list loop)
		// {
		// 	if(output_list[idx].nEvents == counter)
		// 		cacnel;
		// }

		if(QENTRY(tickerId)._order_id)
	 	{

			if(QENTRY(tickerId)._order_id != 0) {
				if(TType == CALL && field == BID) { /* QENTRY(tickerId)._n_call_bid++; */ }	
        			else if(TType == CALL && field == ASK) { /* QENTRY(tickerId)._n_call_ask++; */ }
        			else if(TType == CALL && field == LAST) //ph1
        			{
					// bump counter for complete orders here.. ???
					QENTRY(tickerId)._n_call_last+=1;
					if(QENTRY(tickerId)._n_call_last >= QENTRY(tickerId)._n_events) {
						long qid = QENTRY(tickerId)._order_id;
						ACE_DEBUG((LM_DEBUG,"[%D|%t] Event counter reached max(%d)...cancel order=%d\n", 
							QENTRY(tickerId)._n_events,
							qid));
						(void) m_pClient->cancelOrder(qid);
						complete_transaction(qid,-1,false);
						QENTRY(tickerId)._n_call_last=0;
						QENTRY(tickerId)._cancel_tid=0;
					}
        			}
        			else if(TType == PUT && field == BID) { /* QENTRY(tickerId)._n_put_bid++; */ }
        			else if(TType == PUT && field == ASK) { /* QENTRY(tickerId)._n_put_ask++; */ }
        			else if(TType == PUT && field == LAST) { // ph1 
					QENTRY(tickerId)._n_put_last+=1;
					if(QENTRY(tickerId)._n_put_last >= QENTRY(tickerId)._n_events) {
						long qid = QENTRY(tickerId)._order_id;
						ACE_DEBUG((LM_DEBUG,"[%D|%t] Event counter reached max(%d)...cancel order=%d\n", 
							QENTRY(tickerId)._n_events,
							qid));
						(void) m_pClient->cancelOrder(qid);
						complete_transaction(qid,-1,false);
						QENTRY(tickerId)._n_put_last = 0;
						QENTRY(tickerId)._cancel_tid = 0;
					}
        			}
				else {
					QENTRY(tickerId)._n_other++;
				}
			}
		}

		m_dba.add_record(stime,  			// timestemp (?)
                        m_quotes[m_qmapper[tickerId].theQ]._contract.symbol,	// symbol (from conf)
                        (TType == PUT) ? "P" : "C",				// put/call
                        price,							// price relative to bid/ask/trade
                        size,							// volume
                        m_quotes[m_qmapper[tickerId].theQ]._contract.strike, 	// strike price (from conf)
                        X[field].name,
			tickerId,
			"options_data");						// bid/ask/last(trade)

	}
	else if(QENTRY(tickerId)._contract.secType == "STK") {

		//ACE_DEBUG((LM_DEBUG,"[%D] STK CHECK CID=%d\n", QENTRY(tickerId)._cancel_tid));

		if(QENTRY(tickerId)._cancel_tid) 
		{	
			switch(field)
			{
				case ASK:
				break;

				case BID: 
				break;

				case LAST:
					
					QENTRY(tickerId)._n_stk_last++;

					if(QENTRY(tickerId)._n_stk_last >= QENTRY(tickerId)._n_events)
					{
						// nEvents reached... new req...
						// if the current order is BUY, just cancel
						// if the current pending order is SELL cacnel this and place new SELL for MKT
						// How do i know what is the current orders.. LAST ???
						//
						long oid = QENTRY(tickerId)._order_id;
						ACE_DEBUG((LM_DEBUG,"[%D|%t] [STK], TRADE EVT cnt reached max(%d)/last(%d)..cancel order=%d\n",
							QENTRY(tickerId)._n_events,
							QENTRY(tickerId)._n_stk_last,
							oid));

						(void) m_pClient->cancelOrder(oid);
						complete_transaction(oid,-1,false);

						QENTRY(tickerId)._n_stk_last=0;
						QENTRY(tickerId)._cancel_tid = 0;
					}	 
				break;

				default:
					QENTRY(tickerId)._n_other++;
				break;
			
			}
		}

		m_dba.add_record(stime,  			// timestemp (?)
                        m_quotes[m_qmapper[tickerId].theQ]._contract.symbol,	// symbol (from conf)
                        price,							// price relative to bid/ask/trade
                        size,							// volume
                        X[field].name,
			tickerId,
			"stock_data");						// bid/ask/last(trade)
	}

        return;

}



void IB_Client::cancelOrder(long* qid)
{
	if(!qid) return; 

	MYSQL_THREAD_REGISTER;

	ACE_DEBUG((LM_DEBUG, "[%D|%t] Execute 'cancelOrder' for qid=[%d]\n", *qid));

	(void) m_pClient->cancelOrder(*qid);	

	char qid_value[32]; 
	sprintf(qid_value,"%ld", *qid);

	// LOCK
	//m_db_orders.remove_record("orderId",qid_value,"pending_orders");
	complete_transaction(*qid,-1,false);

	delete qid;
}


void IB_Client::placeOrder(string external_id)
{
	Contract		contract;
	Order			order;
	OrderId			id = m_next_id;
	
	MYSQL_THREAD_REGISTER;

	struct cInfo 	{ long cancelInterval; } 	s_cinfo;
	struct cId 	{ long id; } 			cid;
	struct eTime    { string execTime; } 		etime;
	struct tickerID { long ticker_id; } 		tickerid;
	struct NEvents  { long nEvents; } 		nevents;

	ACE_DEBUG((LM_DEBUG,"[%D|%t] Prepare to place a new Order ExtId=%s\n", external_id.c_str()));
	
	vector<pair<string,string> > result;
	bool r = m_db_orders.get_record("id",external_id,result,"orders_queue");
	if(!r) 
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to retvieve information for placing a new order [%s]\n", external_id.c_str()));
		return;
	}

	if(result.size() == 0)
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to retieve info for a new order. Empty vector EID[%s]\n", external_id.c_str()));
		return;
	}

#define LOAD_DATA(OBJ,KEY)       if(result[idx].first == #KEY) \
		{ OBJ.KEY = (result[idx].second != "NULL") ? result[idx].second : ""; }

#define LOAD_DATA_INT(OBJ,KEY)   if(result[idx].first == #KEY) \
		{ OBJ.KEY = (result[idx].second != "NULL") ? ::atoi(result[idx].second.c_str()) : 0; }

#define LOAD_DATA_LONG(OBJ,KEY)  if(result[idx].first == #KEY) \
		{ OBJ.KEY = (result[idx].second != "NULL") ? ::atol(result[idx].second.c_str()) : 0; }

#define LOAD_DATA_DBL(OBJ,KEY)   if(result[idx].first == #KEY) \
		{ OBJ.KEY = (result[idx].second != "NULL") ? ::atof(result[idx].second.c_str()) : 0; }

#define LOAD_DATA_BOOL(OBJ,KEY)  if(result[idx].first == #KEY) \
		{ OBJ.KEY = ((result[idx].second != "NULL") ? ((result[idx].second == "TRUE") ? true : false) : false); }

	for(size_t idx = 0; idx < result.size(); idx++)
	{

		LOAD_DATA_INT(cid,id);
		LOAD_DATA_LONG(s_cinfo,cancelInterval);
		LOAD_DATA_LONG(tickerid,ticker_id); 
		LOAD_DATA_LONG(nevents, nEvents);
		//LOAD_DATA(etime,execTime);
		//fprintf(stderr," ***** ID=%ld : TID=%ld : nEvents=%ld********\n", 
		//		cid.id, tickerid.ticker_id, nevents.nEvents);
		//ACE_DEBUG((LM_DEBUG,"[%D|%t] - ExtID=%d, TID=%d, nEvents=%d\n",
		//		cid.id, tickerid.ticker_id, nevents.nEvents));

		QENTRY(tickerid.ticker_id)._n_events  = nevents.nEvents;
		QENTRY(tickerid.ticker_id)._order_id  = id;

		//contract
		LOAD_DATA(contract,symbol);
		LOAD_DATA(contract,secType);
		LOAD_DATA(contract,expiry);
		LOAD_DATA(contract,right);
		LOAD_DATA(contract,multiplier);
		LOAD_DATA(contract,exchange);
		LOAD_DATA(contract,primaryExchange);
		LOAD_DATA(contract,currency);
		LOAD_DATA(contract,localSymbol);
		//LOAD_DATA(contract,secIdType);
		//LOAD_DATA(contract,secId);	
		//LOAD_DATA(contract,comboLegsDescrip);	
		LOAD_DATA_DBL(contract,strike);
		//LOAD_DATA_BOOL(contract,includeExpired);
		//LOAD_DATA_LONG(contract,conId);

		// order
		//LOAD_DATA_LONG(order,orderId);
		LOAD_DATA_LONG(order,clientId);
		//LOAD_DATA_LONG(order,permId);
		LOAD_DATA(order,action);
		LOAD_DATA_LONG(order,totalQuantity);
		LOAD_DATA(order,orderType);

		LOAD_DATA_DBL(order,lmtPrice);
		LOAD_DATA_DBL(order,auxPrice);

		LOAD_DATA(order,tif);
		//LOAD_DATA(order,ocaGroup);
		//LOAD_DATA_INT(order,ocaType);
		//LOAD_DATA(order,orderRef);
		LOAD_DATA_BOOL(order,transmit);

		//LOAD_DATA_LONG(order,parentId);

		//LOAD_DATA_BOOL(order,blockOrder);
		//LOAD_DATA_BOOL(order,sweepToFill);
		//LOAD_DATA_INT(order,displaySize);
		//LOAD_DATA_INT(order,triggerMethod);
		//LOAD_DATA_BOOL(order,outsideRth);
		//LOAD_DATA_BOOL(order,hidden);
		//LOAD_DATA(order,goodAfterTime);
		//LOAD_DATA(order,goodTillDate);
		//LOAD_DATA(order,rule80A);

		//LOAD_DATA_BOOL(order,allOrNone);
		LOAD_DATA_INT(order,minQty);
		//LOAD_DATA_DBL(order,percentOffset);
		//LOAD_DATA_BOOL(order,overridePercentageConstraints);
		LOAD_DATA_DBL(order,trailStopPrice);
		LOAD_DATA_DBL(order,trailingPercent);

		//LOAD_DATA(order,faGroup);
		//LOAD_DATA(order,faProfile);
		//LOAD_DATA(order,faMethod);
		//LOAD_DATA(order,faPercentage);

		LOAD_DATA(order,openClose);
		///LOAD_DATA(order,origin);
		LOAD_DATA_INT(order,shortSaleSlot);

		LOAD_DATA(order,account);
	}

	order.orderId = id;
	
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Placing Order[%d:%s:%s:%s:%s:%.2f:Qty(%d)]\n", 
			order.orderId,
			contract.symbol.c_str(),
			contract.exchange.c_str(),
			contract.secType.c_str(),
			order.action.c_str(),
			order.lmtPrice,
			order.totalQuantity));

	char s_oid[32], s_id[32], t_id[32];;
	sprintf(s_oid,"%ld", order.orderId);
	sprintf(s_id, "%ld", cid.id);
	sprintf(t_id, "%ld", tickerid.ticker_id);

	//update recored here ???
	// LOCK
	if(!m_db_orders.update_record(
			"orderId", s_oid,
			"id", s_id,
			"orders_queue"))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to update OrderId recored for [%d], oid=[%d]\n", cid.id, order.orderId));
	}
	ACE_DEBUG((LM_DEBUG,"[%D|%t] After update_record [s_id=%s,oid=%s]\n", s_id, s_oid));

	// update timestemp
	time_t tt = time(NULL);
       	struct tm ttm = *localtime(&tt);
	char stime[32];
       	sprintf(stime,"'%d-%02d-%02d %02d:%02d:%02d'", ttm.tm_year + 1900, ttm.tm_mon + 1, ttm.tm_mday, ttm.tm_hour, ttm.tm_min, ttm.tm_sec);

	// LOCK
	if(!m_db_orders.update_record(
			"execTime", stime,
			"id", s_id,
			"orders_queue"))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to update TimeStamp for [%d]\n", order.orderId));
	}

//#if 0 // TMP
	m_pClient->placeOrder(id,contract,order);
//#endif

	// start timer for each order placed
	//
	if(s_cinfo.cancelInterval)
	{
		ACE_Time_Value tm(s_cinfo.cancelInterval);
		long *qvalue = new (std::nothrow) long;
		*qvalue = order.orderId;
		long tid = m_timer.start_timer(tm,qvalue);
		if(tid < 0)
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to start timer for ID=%d, interval=%d\n", *qvalue, s_cinfo.cancelInterval));
		}
		// set Timer ID
		QENTRY(tickerid.ticker_id)._cancel_tid = tid;
		m_timer_id_map[id] = tid;	
		ACE_DEBUG((LM_DEBUG,"[%D|%t] Start timer TID=%d, interval=%d\n", tid, s_cinfo.cancelInterval));
	}
	//}
	id++; m_next_id = id;
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Moving record to pending_orders....EID=%s\n",external_id.c_str()));
	if(!m_db_orders.move_record1("id",external_id))
	{
		ACE_DEBUG((LM_ERROR,"[%D|%t] Faield to move an order to pending queue, EID=%s\n",external_id.c_str()));
	}
	// request update from server for order state change... 
	m_pClient->reqOpenOrders();
}

void IB_Client::distribute(int id, int idx)
{
	//fprintf(stderr,"in distribute... FD=%d\n", this->m_wr_fd);
}


/*
 *
 *
 *	1 = bid
 	2 = ask
 	4 = last
 	6 = high
 	7 = low
 	9 = close
 *
 *
 */

void IB_Client::tickSize( TickerId tickerId, TickType field, int size) 
{
	/*
        struct { int id; char name[10]; } X[] =
        {
                { 0, "BID_SIZE" },
                { 1, "NA" },
                { 2, "NA" },
                { 3, "ASK_SIZE" },
                { 4, "NA"},
                { 5, "LAST_SIZE" },
                { 6, "NA" },
                { 7, "NA" },
                { 8, "NA" },
                { 9, "NA" },
                { 10, "END" }
        };

	fprintf(stderr,"Received: [%s]: ID=[%ld],field=[%d]:[%s],size=[%d]\n\n\n",__FUNCTION__,tickerId,field,X[field].name,size);
	*/
}

void IB_Client::tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
                                       double optPrice, double pvDividend,
                                      double gamma, double vega, double theta, double undPrice) 
{
	//fprintf(stderr,"Received: [%s]: ID=[%ld],Type=[%d],implVol=[%g],delta=[%g],optPrice=[%g],pDiv=[%g],gamma=[%g],vega=[%g],theta=[%g],undPrice=[%g]\n",
	//				__FUNCTION__,tickerId,tickType,impliedVol,delta,optPrice,pvDividend,gamma,vega,theta,undPrice);
}

void IB_Client::tickGeneric(TickerId tickerId, TickType tickType, double value) 
{
	//fprintf(stderr,"Received: [%s]: ID=[%ld], type=[%d],value=[%g]\n",__FUNCTION__,tickerId,tickType,value);
}
void IB_Client::tickString(TickerId tickerId, TickType tickType, const IBString& value) 
{
	//fprintf(stderr,"Received: [%s]: ID=[%ld],type=[%d],value=[%s]\n",__FUNCTION__,tickerId,tickType,value.c_str());
}
void IB_Client::tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
                       double totalDividends, int holdDays, const IBString& futureExpiry, 
		       double dividendImpact, double dividendsToExpiry) 
{
	//fprintf(stderr,"Received: [%s]\n",__FUNCTION__);
}
void IB_Client::openOrder( OrderId orderId, const Contract& cnt, const Order& ord, const OrderState& ostate) 
{
	//fprintf(stderr,"RCVD: OpenOrder Confirmation:[%ld:%s:%s]\n", orderId, cnt.symbol.c_str(), ord.action.c_str());
}


void IB_Client::openOrderEnd() {}

void IB_Client::winError( const IBString &str, int lastError) 
{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Received: [%s]\n",__FUNCTION__));
}
void IB_Client::connectionClosed() 
{
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Received: [%s]\n",__FUNCTION__));
}
void IB_Client::updateAccountValue(const IBString& key, const IBString& val,
                                                                                  const IBString& currency, const IBString& accountName) {}
void IB_Client::updatePortfolio(const Contract& contract, int position,
                double marketPrice, double marketValue, double averageCost,
                double unrealizedPNL, double realizedPNL, const IBString& accountName){}
void IB_Client::updateAccountTime(const IBString& timeStamp) {}
void IB_Client::accountDownloadEnd(const IBString& accountName) {}
void IB_Client::contractDetails( int reqId, const ContractDetails& contractDetails) {}
void IB_Client::bondContractDetails( int reqId, const ContractDetails& contractDetails) {}
void IB_Client::contractDetailsEnd( int reqId) {}



/*
 *


IBString     execId;
   IBString     time;
   IBString     acctNumber;
   IBString     exchange;
   IBString     side;
   int         shares;
   double      price;
   int         permId;
   long        clientId;
   long        orderId;
   int         liquidation;
   int         cumQty;
   double      avgPrice;
   IBString    orderRef;
   IBString        evRule;
   double      evMultiplier;

*/

void IB_Client::execDetails( int reqId, const Contract& contract, const Execution& execution) 
{
	ACE_DEBUG((LM_DEBUG,
	"[%D|%t] execDetails() - invoked for:RId=[%d:%s:%d:%s:%s:%d:%.2f]\n",
			reqId, 
			contract.symbol.c_str(), 
			execution.orderId,
			execution.time.c_str(),
			execution.side.c_str(),
			execution.shares,
			execution.price));

	complete_transaction(execution.orderId,execution.shares,true);

	// no clean up.. 
	// move record to complete orders table.. 
}


bool IB_Client::complete_transaction(long order_id,int e_shares, bool executed)
{
	vector <pair <string, string> > result;
	char oid[16];
	sprintf(oid,"%ld",order_id);
	string symbol, secType, multiplier, currency, orderType, tif, action, goodAfterTime, goodTillDate;
        long clientId, totalQuantity;
        double lmtPrice;
        int minQty; 
	long tid;
	static long m_eid = 65000;
	
	//pthread_spin_lock(&m_db_spin);
	bool r = m_db_orders.get_record("orderId", oid, result);
	//pthread_spin_unlock(&m_db_spin);
	if(!r) {
		ACE_DEBUG((LM_ERROR,"[%D|%t] ERR: OrderId=[%d] wasn't found as pending...\n",order_id));
		return false;
	}

	if(result.size() == 0) {
		ACE_DEBUG((LM_ERROR,"[%D|%t] ERR: OrderId=[%d] seems found, but no data in a record. Internal error. check DB\n",order_id));
		return false;
	}

	ACE_DEBUG((LM_DEBUG,"[%D|%t] in Comp_Order: OID=%s, size=%d\n", oid, result.size()));
	long timer_id = m_timer_id_map[order_id];
	const void* parg;
	ACE_DEBUG((LM_DEBUG,"[%D|%t] Stop Timer Now.. ID=%d\n", timer_id));
	if(m_timer.stop_timer(timer_id, &parg))
	{
		long* poid = (long*)parg;
		if(poid) delete poid;
	}

	long tick_id;
	m_timer_id_map[order_id] = 0;

	// 	"id", 
	// 	"symbol", 
	// 	"secType", 
	// 	"multiplier", 
	// 	"currency", 
	// 	"clientId", 
	// 	"action", 
	// 	"totalQuantity", 
	// 	"orderType", 
	// 	"tif", 
	// 	"transmit", 
	// 	"goodAfterTime", 
	// 	"goodTillDate", 
	// 	"lmtPrice", 
	// 	"minQty", 
	// 	"cancelInterval", 
	// 	"nEvents", 
	// 	"ticker_id"

	// travres fields of the record for given ordedId 
	for(size_t idx = 0; idx < result.size(); idx++) 
	{
		if(result[idx].first == "ticker_id")
		{
			ACE_DEBUG((LM_DEBUG,"[%D|%t] Ticker ID in the record = %s\n", result[idx].second.c_str()));
			tick_id = atol(result[idx].second.c_str());
			QENTRY(tick_id)._cancel_tid = 0;
			tid = tick_id;
		}
		if(result[idx].first == "action")
		{
			ACE_DEBUG((LM_DEBUG,"[%D|%t] - In compleation function orderType is set to: %s\n", result[idx].second.c_str()));
			action = result[idx].second;
		}
		if(result[idx].first == "symbol")
		{
			symbol = result[idx].second;
		}
		if(result[idx].first == "secType")
		{
			secType = result[idx].second;
		}
		if(result[idx].first == "multiplier")
		{
			multiplier = result[idx].second;
		}
		if(result[idx].first == "currency")
		{
			currency = result[idx].second;
		}
		if(result[idx].first == "orderType")
		{
			orderType = result[idx].second;
		}
		if(result[idx].first == "clientId")
		{
			clientId = atoi(result[idx].second.c_str());
		}
		if(result[idx].first == "totalQuantity")
		{
			totalQuantity = atol(result[idx].second.c_str());
		}
		if(result[idx].first == "goodAfterTime")
		{
			goodAfterTime = result[idx].second;
		}
		if(result[idx].first == "goodTillDate")
		{
			goodTillDate = result[idx].second;
		}
		if(result[idx].first == "lmtPrice")
		{
			lmtPrice = atof(result[idx].second.c_str());
		}
		if(result[idx].first == "minQty")
		{
			minQty = atoi(result[idx].second.c_str());
		}
	}

	//pthread_spin_lock(&m_db_spin);
	if(executed)
	{
		if(totalQuantity)
		{
			char tq[16];
			sprintf(tq,"%ld",totalQuantity);
			totalQuantity-=e_shares;

			if(!m_db_orders.update_record(
					"totalQuantity",		
				 	tq,
					"orderId", 
					oid,
					"pending_orders"))
			{
				ACE_DEBUG((LM_ERROR,"Failed to update total price after partial exec oid=%d\n", order_id));
			}
			ACE_DEBUG((LM_DEBUG,"[%D|%t] Order is not fully complete... q=%d, eshares=%d\n", totalQuantity,e_shares));

			if(!totalQuantity)
			{
				m_db_orders.move_record("orderId",oid,"exec_status","X");
				ACE_DEBUG((LM_DEBUG,"[%D|%t] Pending entry has been removed for order_id [%s]\n", oid));
			}
		}
		else
		{
			m_db_orders.move_record("orderId",oid,"exec_status","X");
			ACE_DEBUG((LM_DEBUG,"[%D|%t] Pending entry has been removed for order_id [%s]\n", oid));
		}
	}
	else
	{
		if(action == "SELL" && orderType == "MKT")
		{
			ACE_DEBUG((LM_DEBUG,"Skip cancell for Sell/Mkt order_id=%d\n",oid));
		}
		else if(action == "BUY")  
		{
			m_db_orders.move_record("orderId",oid,"exec_status","C");
			ACE_DEBUG((LM_DEBUG,"[%D|%t] Pending entry has been removed for canclled order_id [%s]\n", oid));
		}
		else if(action == "SELL" && orderType != "MKT") // this is LMT sell cancelled... 
		{
			if(!m_db_sell.place_orders(
				m_eid, 
				symbol, 
				secType,
				multiplier,
				currency,
				clientId,
				"SELL",
				totalQuantity,
				"MKT",  // <-- MKT sell
				"GTC",
				"TRUE",
				goodAfterTime,
				goodTillDate,
				lmtPrice,
				minQty,
				0, //  no timer
				200,
				tid))
			{
				ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to place SELL Order with MKT place....mEID[%d]\n",m_eid));
				// return false; ???
			}
			m_eid--;
			m_db_orders.move_record("orderId",oid,"exec_status","X");
			ACE_DEBUG((LM_DEBUG,"[%D|%t] mark the original sell order as executed order_id [%s]\n", oid));
			//ACE_DEBUG((LM_DEBUG,"[%D|%t] Placed market sell order to cancel SELL transaction\n"));
		}
	}
	return true;
}


void IB_Client::execDetailsEnd( int reqId) {}

void IB_Client::updateMktDepth(TickerId id, int position, int operation, int side,
                                                                          double price, int size) {}
void IB_Client::updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
                                                                                int side, double price, int size) {}
void IB_Client::updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch) {}
void IB_Client::managedAccounts( const IBString& accountsList) {}
void IB_Client::receiveFA(faDataType pFaDataType, const IBString& cxml) {}
void IB_Client::scannerParameters(const IBString &xml) {}
void IB_Client::scannerData(int reqId, int rank, const ContractDetails &contractDetails,
           const IBString &distance, const IBString &benchmark, const IBString &projection,
           const IBString &legsStr) {}
void IB_Client::scannerDataEnd(int reqId) {}
void IB_Client::fundamentalData(TickerId reqId, const IBString& data) {}
void IB_Client::deltaNeutralValidation(int reqId, const UnderComp& underComp) {}
void IB_Client::tickSnapshotEnd(int reqId) 
{

	ACE_DEBUG((LM_DEBUG,"Received: [%s]\n",__FUNCTION__));
}
void IB_Client::marketDataType(TickerId reqId, int marketDataType) 
{

	ACE_DEBUG((LM_DEBUG,"Received: [%s]\n",__FUNCTION__));
}
void IB_Client::commissionReport( const CommissionReport& commissionReport) {}


