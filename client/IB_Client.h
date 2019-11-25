#ifndef _IB_CLIENT_H_
#define _IB_CLIENT_H_


#include "Timer.h"

#include "EWrapper.h"
#include <memory>
#include <list>
#include <stdio.h>

//#include <zmq.h>
#include <Config.h>

#include <DataStore.h>
#include <Executor.h>
#include <IB_Processor.h>

#include <Message.h>
#include <DBAccess.h>

#include <ace/Log_Msg.h>


#define CLIENT_VERSION 59

const char FIFO[]="./.channel";
const char PORD[]="/tmp/.porder";
const int COLL_COMPLT=6;

enum app_State
{
	STATE_UNDEF = 0,
	STATE_SEND_RMD = 1,
	STATE_SEND_RTB = 2,
	STATE_SEND_RHD = 3,
	STATE_RMD_SENT = 4,
	STATE_RMD_RCVD = 5,
	STATE_RTB_SENT = 6,
	STATE_RTB_RCVD = 7,
	STATE_RHD_SENT = 8,
	STATE_RHD_RCVD = 9
};


enum 
{
	PUT = 0,
	CALL = 1
};
	

class EPosixClientSocket;

const int NUM_OF_ELEMENTS=200;

class IB_Client : public EWrapper
{
	_s_message	_msg;
public:
	IB_Client(Config& cf,int id);
	~IB_Client();

	bool init_data_store();
	bool open_channel();
	bool read_channel();


	bool connect(const char* host, unsigned int port);
	void disconnect() const;
	bool isConnected() const;

	static void* starter(void* arg) {
                //std::cerr << "Thread starter called" << std::endl;
                return reinterpret_cast<IB_Client*>(arg)->executor(arg);
        }

	void* executor(void*);
	void distribute(int id, int idx);

	bool complete_transaction(long order_id,int e_shares, bool executed);


	/* private calls */
	void reqCurrentTime(void);
	void placeOrder(string external_id);
	void cancelOrder(long* qid);
	void reqOpenOrders();


	void reqMarketData(void);
	void reqRealTimeBars(void);
	void reqHistData(void);

	void processMessages(); 

	/* overloaded */
	void tickPrice(TickerId tickerId, TickType field, double price, int canAutoExecute);
	void tickPriceEx(TickerId tickerId, TickType field, double price, int canAutoExecute, TickType, int);
	void tickSize(TickerId tickerId, TickType field, int size);
	void tickOptionComputation( TickerId tickerId, TickType tickType, double impliedVol, double delta,
		double optPrice, double pvDividend, double gamma, double vega, double theta, double undPrice);
	void tickGeneric(TickerId tickerId, TickType tickType, double value);
        void tickString(TickerId tickerId, TickType tickType, const IBString& value);
        void tickEFP(TickerId tickerId, TickType tickType, double basisPoints, const IBString& formattedBasisPoints,
                double totalDividends, int holdDays, const IBString& futureExpiry, double dividendImpact, double dividendsToExpiry);
        void orderStatus(OrderId orderId, const IBString &status, int filled,
                int remaining, double avgFillPrice, int permId, int parentId,
                double lastFillPrice, int clientId, const IBString& whyHeld);
        void openOrder(OrderId orderId, const Contract&, const Order&, const OrderState&);
        void openOrderEnd();
        void winError(const IBString &str, int lastError);
        void connectionClosed();
        void updateAccountValue(const IBString& key, const IBString& val,
                const IBString& currency, const IBString& accountName);
        void updatePortfolio(const Contract& contract, int position,
                double marketPrice, double marketValue, double averageCost,
                double unrealizedPNL, double realizedPNL, const IBString& accountName);
        void updateAccountTime(const IBString& timeStamp);
        void accountDownloadEnd(const IBString& accountName);
        void nextValidId(OrderId orderId);
        void contractDetails(int reqId, const ContractDetails& contractDetails);
        void bondContractDetails(int reqId, const ContractDetails& contractDetails);
        void contractDetailsEnd(int reqId);
        void execDetails(int reqId, const Contract& contract, const Execution& execution);
	void execDetailsEnd(int reqId);
        void error(const int id, const int errorCode, const IBString errorString);
        void updateMktDepth(TickerId id, int position, int operation, int side,
                double price, int size);
        void updateMktDepthL2(TickerId id, int position, IBString marketMaker, int operation,
                int side, double price, int size);
        void updateNewsBulletin(int msgId, int msgType, const IBString& newsMessage, const IBString& originExch);
        void managedAccounts(const IBString& accountsList);
        void receiveFA(faDataType pFaDataType, const IBString& cxml);
        void historicalData(TickerId reqId, const IBString& date, double open, double high,
                double low, double close, int volume, int barCount, double WAP, int hasGaps);
        void scannerParameters(const IBString &xml);
        void scannerData(int reqId, int rank, const ContractDetails &contractDetails,
                const IBString &distance, const IBString &benchmark, const IBString &projection,
                const IBString &legsStr);
        void scannerDataEnd(int reqId);
        void realtimeBar(TickerId reqId, long time, double open, double high, double low, double close,
                long volume, double wap, int count);
        void currentTime(long time);
	 void fundamentalData(TickerId reqId, const IBString& data);
        void deltaNeutralValidation(int reqId, const UnderComp& underComp);
        void tickSnapshotEnd(int reqId);
        void marketDataType(TickerId reqId, int marketDataType);
        void commissionReport( const CommissionReport& commissionReport);

	// This objects will contain all sec data + executor for 3p processing routine
	typedef DataStore<Quotes,NUM_OF_ELEMENTS>	DSTORE;
	typedef Executor<Quotes,Process>		PROC;

private:
	TickerId	m_id;


	app_State				m_state; // application collection mode. reqHistData, ReqRTBars, ReqMarketData... 

	Config					m_conf;
	int					m_clientId;		// client ID is associated with INI provisiong. App will assign only Quotes 
									// dedicated for the clientId value in INI file. The rest will be ignored
	std::vector<Config::SData>		m_quotes;		// vector contain actual Sec Data objects associted with same cliendId in INI

	// Internal mapper structure maps QID or TickerId
	// with the currently processing Quotes structure. 
	struct sMapper
	{
		int 				theQ;		// q index during initilization
		Quotes*				qCurrent;	// Pointes to the Quotes object currently in collection state
		int				type;		// Type of the data collecting: PUT or CALL
		PROC* 				exec;		// Executor for specific secutiry
		DSTORE*				dstore[2];	// datastore associted with specific security (2 entires points to the same Q object)
		Quotes* 			qStart;
		int				d_index;	// currently used store... 
		TickerId			sib_id;		// siblings TID
	};

	enum eOrderStates
	{
		OPER_STATE_INVALID 		= 0, 	// Initial state
		OPER_STATE_OPEN_PENDING 	= 1, 	// INIT -> place_order() completed
		OPER_STATE_OPEN_CONFIRMED 	= 2, 	// received confiramtion from TWS
		OPER_STATE_CANCELED 		= 3,	// after timer expired or max last reached
		OPER_STATE_FULFILLED 		= 4,	// order executed. after execDetails()
		OPER_STATE_ARCH			= 5,	// order archived for further review.. 
	};

	struct sOrderOperData
	{
		eOrderStates			m_state;
		OrderId				m_order_id;
		long				m_id;		
		long				m_timer_id;
		long				m_event_counter; 
	};

	std::map<OrderId, sOrderOperData>	m_orders;
	typedef std::map<OrderId, sOrderOperData>::iterator order_iter;


	bool add_new_order(OrderId oid)
	{
		sOrderOperData oper;
		oper.m_state = OPER_STATE_INVALID;
		oper.m_order_id = oid;
		oper.m_timer_id = -1;
		oper.m_event_counter = 0;	
		m_orders.insert(std::map<OrderId, sOrderOperData>::value_type(oid, oper));
		return true;
	}

	sOrderOperData&  get_order_details(OrderId oid)
	{
		order_iter it = m_orders.find(oid);
		if(it != m_orders.end())
		{
			ACE_DEBUG((LM_ERROR,"[%D|%t] Failed to retrieve Order Data by OrderData\n"));
		}
		return it->second;
	}

	bool remove_order(OrderId oid)
	{
		m_orders.find(oid);
		return true;
	}

	int					m_rd_fd;
	int					m_wr_fd;
	pthread_t				m_chl_tid;

	sMapper					m_qmapper[128]; // mapping array QID -> TYPE + Quotes Object pointer currently in use	
	std::auto_ptr<EPosixClientSocket> 	m_pClient;	// IB Socket implementation. 

	DB_Access				m_dba;
	DB_Access				m_db_orders;
	DB_Access				m_db_sell;

	OrderId 				m_next_id;

	IBC_Timer				m_timer;

	pthread_spinlock_t 			m_db_spin;

	vector<long>				m_timer_id_map;

};



#endif
