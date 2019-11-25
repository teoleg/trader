#ifndef _DB_ACCESS_LAYER_H_
#define _DB_ACCESS_LAYER_H_

#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;


#include <my_global.h>
#include <mysql.h>

#include <pthread.h>

class DB_Access
{
	MYSQL*				_con;
	std::string			_host;
	std::string			_user;
	std::string			_passwd;
	std::string			_db_name;

	pthread_spinlock_t              _lock;

	class myGuard {
		pthread_spinlock_t	*_lock;
	public:
		myGuard(pthread_spinlock_t* p):_lock(p) {
			if(_lock) pthread_spin_lock(_lock);
		}
		~myGuard() {
			if(_lock) pthread_spin_unlock(_lock); 
		}
	};

#define MY_QUARD(LOCK) myGuard grd(LOCK);

public:
	typedef map<string,string>      PLIST;
	typedef map<string, PLIST>       ORDERS;

	DB_Access(std::string host="localhost",
		  std::string user="root",
		  std::string pwd="jopa88jopa",
		  std::string db="trade_data")
	:
	 _con(0),
	 _host(host),
	 _user(user),
	 _passwd(pwd),
	 _db_name(db)
	{
		
	}

	~DB_Access()
	{
		mysql_close(_con);
	}


	bool init();
	void fini();

	bool add_record(
			string time,		// time             - varchar(32)
			string underline,		// underline	    - varchar(8)
			string callput,		// callput	    - varchar(1), values: 'C'-call, 'P'-put, 'S'-stock
			double 		price,		// price	    - double
			int 		volume,		// volume	    - int(11)
			double		strike,		// strike	    - int(11)
			string qtype,		// quote_type	    - varchar(2), values: 'T'-trade, 'A'-ask, 'B'-bid
			int ticker_id,			// ticker id
			string tbl = "options_data"		
			);

	bool add_record(
			string time,
			string underline,
			double	price,
			int	volume,
			string qtype,		
			int	ticker_id,
			string tbl = "stock_data"
			);
			


	bool add_record_stock(
				string time,
				string symbol,
				string tr_type, // "trade" or "quote"
				double tr_price, 
				int tr_volume,
				int cum_volume,
				double bid_price,
				int bid_size,
				double ask_price,
				int ask_size,
				string tbl = "tick" 
			);

	bool move_all_records(string from, string to);

	bool move_record(string lookupkey, 
			string lookupvalue, 
			string upkey = "",
			string upvalue = "",
			string from_tbl = "pending_orders", 
			string to_tbl = "complete_orders");

	bool move_record1(string lookupkey, 
			string lookupvalue, 
			string from_tbl = "orders_queue", 
			string to_tbl = "pending_orders");

	bool remove_record(string key, string value, string tbl);

	bool update_record(string setkey, string setvalue, string lookupkey, string lookupvalue, string tbl);

	bool get_record(string lookupkey, string lookupvalue, vector<pair<string,string> > &result, string tbl = "pending_orders");

	bool get_records(vector<pair<string,string> > &out,string id="*",string tbl="tick");

	bool get_orders(ORDERS & out, string tbl = "orders_queue"); 

	bool get_all_recods_by_id(ORDERS& out, string lookupkey, string lookupvalue, string tbl="pending_orders");

	int get_nrecords(string tbl="tick") const; 

	bool get_orders_record(string tbl="orders_queue");

	bool place_orders(int id,
                          string symbol,
                          string secType,
                          string ultiplier,
                          string currency,
                          long clientId,
                          string action,
                          long totalQuantity,
                          string rderType,
                          string tif,
                          string transmit,
                          string goodAfterTime,
                          string goodTillDate,
                          double lmtPrice,
                          int minQty,
                          long cancelInterval,
                          long nEvents,
                          long tid);

};



#define LLOAD_DATA(OBJ,KEY) \
        { \
        OBJ.KEY = (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second != "NULL") ? \
                ((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second : "" ; }

#define LLOAD_DATA_DBL(OBJ,KEY) \
        { \
        OBJ.KEY = (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second != "NULL") ? \
                ::atof(((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second.c_str()) : 0.0 ; }

#define LLOAD_DATA_LONG(OBJ,KEY) \
        { \
        OBJ.KEY = (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second != "NULL") ? \
                ::atol(((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second.c_str()) : 0 ; }

#define LLOAD_DATA_BOOL(OBJ,KEY) \
        { \
        OBJ.KEY = (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second != "NULL") ? \
                (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second == "TRUE" ? true : false ) : false ; }

#define LLOAD_DATA_INT(OBJ,KEY) \
        { \
        OBJ.KEY = (((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second != "NULL") ? \
                ::atoi(((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find(#KEY))->second.c_str()) : 0 ; }



#endif

