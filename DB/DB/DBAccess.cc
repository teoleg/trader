#include <DBAccess.h>
#include <poll.h>

bool DB_Access::init()
{
	if(_con)
	{
		std::cerr << "DBA:init() - Connection is already established...exit" << std::endl;
		return false;
	}

	_con = mysql_init(NULL);
	if(!_con)
	{
		std::cerr << "DBA::init() - Failed to connect to DB " << _db_name << std::endl;
		return false;
	}

	if(mysql_real_connect(_con,_host.c_str(),_user.c_str(),_passwd.c_str(),_db_name.c_str(),0,NULL,0) == NULL)
	{	
		std::cerr << "DBA:init() - Failed to connect to DB=" << _db_name << std::endl;
		_con = 0;
		return false;
	}

	my_bool reconnect = 1;
	mysql_options(_con,MYSQL_OPT_RECONNECT, &reconnect);
	mysql_options(_con,MYSQL_INIT_COMMAND,"SET autocommit=1");

	if(pthread_spin_init(&_lock, 0) < 0)
	{
		std::cerr << "DBA:init() - falied to init spinlock" << std::endl;
		return false;
	}
	return true;
}

void DB_Access::fini()
{
	if(!_con)
	{
		std::cerr << "DBA:fini() - invalid connection..exit" << std::endl;
		return;
	}
	mysql_close(_con);
	_con = NULL;
}

bool DB_Access::add_record(
		std::string time,		// time             - varchar(32)
		std::string underline,		// underline	    - varchar(8)
		std::string callput,		// callput	    - varchar(1), values: 'C'-call, 'P'-put, 'S'-stock
		double price,			// price	    - double
		int volume,		// volume	    - int(11)
		double strike,				// strike	    - int(11)
		std::string qtype,		// quote_type	    - varchar(2), values: 'T'-trade, 'A'-ask, 'B'-bid
		int ticker_id,
		std::string tbl		
		)
{
	char prc[16]; 
	sprintf(prc,"%f", price);
	char srk[16];
	sprintf(srk,"%f", strike);
	char tid[32];
	sprintf(tid,"%d", ticker_id);
	char vol[64];
	sprintf(vol,"%d",volume);

	std::string query = "INSERT INTO " + 
		tbl + 
		"(time,underline,callput,price,volume,strike,quote_type,ticker_id) values (" +
		+ "'" + time + "'" + "," + 
		+ "'" + underline + "'" + "," + 
		+ "'" + callput + "'" + "," + 
		prc + "," +
		vol + "," + 
		srk + "," +
		+ "'" + qtype + "'" + "," + tid + ");";

		//std::cerr << "DBA::ar_opt()[" << std::endl << query << std::endl << "]" << std::endl;

        	mysql_ping(_con);
		if(mysql_query(_con, query.c_str()))
		{
			std::cerr << "DBA::ar_opt() - Failed to update table: " << tbl << " " << std::endl;
			std::cerr << "DBA::ar_opt(ERROR) - " << mysql_error(_con) << std::endl;
			return false;
		}

		return true;
			
}

bool DB_Access::add_record(
                        std::string time,
                        std::string underline,
                        double  price,
                        int     volume,
			std::string qtype,		
                        int     ticker_id,
                        std::string tbl
                        )
{
	char prc[16]; 
	sprintf(prc,"%f", price);
	char tid[32];
	sprintf(tid,"%d", ticker_id);
	char vol[64];
	sprintf(vol,"%d",volume);

	std::string query = "INSERT INTO " + 
		tbl + 
		"(time,underline,price,volume,quote_type,ticker_id) values (" +
		+ "'" + time + "'" + "," + 
		+ "'" + underline + "'" + "," + 
		prc + "," +
		vol + "," + 
		+ "'" + qtype + "'" + "," + tid + ");";

		//std::cerr << "DBA::ar_stock()[" << std::endl << query << std::endl << "]" << std::endl;

                mysql_ping(_con);
		if(mysql_query(_con, query.c_str()))
		{
			std::cerr << "DBA::ar_stock() - Failed to update table: " << tbl << " " << std::endl;
			std::cerr << "DBA::ar_stock(ERROR) - " << mysql_error(_con) << std::endl;
			return false;
		}

		return true;

}

bool DB_Access::get_all_recods_by_id(ORDERS& out, string lookupkey, string lookupvalue, string tbl)
{
	if(!_con)
        {
                std::cerr << "DBA:get_all_recods_by_id() - invalid connection..exit" << std::endl;
                return false;
        }	

	string query;
	query = "SELECT * FROM " + _db_name + "." + tbl + " WHERE " + lookupkey + "=" + lookupvalue + ";";

	return true;
}


bool DB_Access::get_orders(ORDERS& out, string tbl)
{
	if(!_con)
        {
                std::cerr << "DBA:get_r() - invalid connection..exit" << std::endl;
                return false;
        }

	std::string query;
	query = "SELECT * FROM " + _db_name + "." + tbl + " ORDER BY id LIMIT 1;";

	mysql_ping(_con);
	if(mysql_query(_con, query.c_str()))
	{
		std::cerr << "DBA:get_orders() - Q[" << query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}
	cerr << "get_orders() - Q[" << query << "]" << endl;
	
	
	MYSQL_RES* res = mysql_store_result(_con);
	if(!res)
	{
		std::cerr << "DBA:get_r() - DBERROR:" << mysql_error(_con) << std::endl;
		return false;
	}
	int nf = mysql_num_fields(res);
	std::cerr << "DBA::get_r() - received #" << nf << std::endl;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	int retry = 3;
	do
	{
		while(row = mysql_fetch_row(res))
		{
			string cID;
			PLIST pl;
			cerr << "I'm in the loop fetching params.." << endl;
			for(int i = 0; i < nf; i++)
			{
				field = mysql_fetch_field_direct(res,i);
				//cerr << "Field ID=" << i << endl;
				if(i == 0) // ID
				{
					cID = row[i];
					//std::cerr << "ID=" << cID << ", where i = " << i <<  endl;
					//std::cerr << "Key=[" << field->name << "], value=[" << (row[i] ? row[i] : "NULL") << "]" << std::endl;
					pl.insert(PLIST::value_type(field->name,(row[i] ? row[i] : "NULL")));
				}
				else
				{
				//std::cerr << "Key=[" << field->name << "], value=[" << (row[i] ? row[i] : "NULL") << "]" << std::endl;
					pl.insert(PLIST::value_type(field->name,(row[i] ? row[i] : "NULL")));
				}
			}
			out.insert(ORDERS::value_type(cID, pl));
			//std::cerr << std::endl;
		}
		if(out.size())
		    retry = 0;
		else 
		{
		    cerr << "-------- HIT ERROR CONDITION ---- RETRY ONE MORE TIMR ----------" << endl;
		    poll(0,0,200);
		    retry--;
		}
	}
	while(retry);

	mysql_free_result(res);
	return true;
}

bool DB_Access::move_all_records(string from_tbl, string to_tbl)
{
	
	if(!_con)
	{
		std::cerr << "DBA:get_r() - invalid connection..exit" << std::endl;
		return false;
	}
	//INSERT INTO pending_orders SELECT * from orders_queue;
	//DELETE FROM `trade_data`.`orders_queue`;
	std::string move_query = "INSERT INTO " + _db_name + "." + to_tbl + " SELECT * FROM " + _db_name + "." + from_tbl + ";";
	std::string dlt_query  = "DELETE FROM " + _db_name + "." + from_tbl + ";";

        mysql_ping(_con);
	if(mysql_query(_con,move_query.c_str()))
	{
		std::cerr << "DBA:move_record() - Q[" << move_query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}
	//sleep(1);
        mysql_ping(_con);
	if(mysql_query(_con,dlt_query.c_str()))
	{
		std::cerr << "DBA:move_record() - Q[" << dlt_query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}
	return true;
}

bool DB_Access::move_record1(string lookupkey, string lookupvalue, string from_tbl, string to_tbl)
{
	if(!_con)
	{
		std::cerr << "DBA:move_record() - invalid connection..exit" << std::endl;
		return false;
	}

	MY_QUARD(&_lock);

	// Q = INSERT INTO <to table> SELECT * FROM <from table> WHERE 'lookupkey'='lookupvalue' ON DUPLICATE KEY UPDATE  'upkey'='upvalue';
	//INSERT INTO trade_data.complete_orders SELECT * FROM trade_data.pending_orders WHERE orderId=101 ON DUPLICATE KEY UPDATE exec_status=C;
	string insert_query = 
	"INSERT INTO " + _db_name + "." + to_tbl + " SELECT * FROM " + _db_name + "." + from_tbl + " WHERE " + lookupkey + "=" + lookupvalue + ";";
	cerr << "move_record 1st: [" << insert_query << "]" << endl; 
	
	mysql_ping(_con);
	if(mysql_query(_con,insert_query.c_str()))
	{
		std::cerr << "DBA:move_record() - Q[" << insert_query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}

	std::string delete_query = 
	"DELETE FROM " + _db_name + "." + from_tbl + " WHERE " + lookupkey + "=" + lookupvalue + ";";
	std::cerr << "move_record 2nd: [" << delete_query << "]" << std::endl;

	mysql_ping(_con);
	if(mysql_query(_con,delete_query.c_str()))
	{
		std::cerr << "DBA:remove_record() - Q[" << delete_query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}

	return true;
}


bool DB_Access::move_record(string lookupkey, string lookupvalue, string upkey, string upvalue, string from_tbl, string to_tbl)
{
	if(!_con)
	{
		std::cerr << "DBA:move_record(2) - invalid connection..exit" << std::endl;
		return false;
	}

	MY_QUARD(&_lock);

	// Q = INSERT INTO <to table> SELECT * FROM <from table> WHERE 'lookupkey'='lookupvalue' ON DUPLICATE KEY UPDATE  'upkey'='upvalue';
	//INSERT INTO trade_data.complete_orders SELECT * FROM trade_data.pending_orders WHERE orderId=101 ON DUPLICATE KEY UPDATE exec_status=C;
	//
	//
	int retry = 5;
	do
	{
		string insert_query = 
		"INSERT INTO " + _db_name + "." + to_tbl + " SELECT * FROM " + _db_name + "." + from_tbl + " WHERE " + lookupkey + "=" + lookupvalue + ";";
		cerr << "move_record 1st: [" << insert_query << "]" << endl; 
		mysql_ping(_con);
		if(mysql_query(_con,insert_query.c_str()))
		{
			retry--;
			std::cerr << "DBA:move_record(2) - Q[" << insert_query << "] failed with error: [" 
				<< mysql_error(_con) << "], retry " << retry << std::endl;
			::poll(0,0,200);
			continue;
		}
		else
			retry = 0;
	}
	while(retry);

	retry = 5;
	do
	{

		string update_query = 
		"UPDATE " + _db_name + "." + to_tbl + " SET " + upkey + "='" + upvalue + "' WHERE " + lookupkey + "=" + lookupvalue + ";";
		cerr << "move_record 2nd: [" << update_query << "]" << endl; 

		mysql_ping(_con);
		if(mysql_query(_con,update_query.c_str()))
		{
			retry--;
			std::cerr << "DBA:move_record(2) - Q[" << update_query << "] failed with error: [" 
					<< mysql_error(_con) << "], retry" << retry << std::endl;
			::poll(0,0,200);
			continue;
		}
		else retry=0;
	}
	while(retry);

	retry = 5;

	do
	{
		std::string delete_query = 
		"DELETE FROM " + _db_name + "." + from_tbl + " WHERE " + lookupkey + "=" + lookupvalue + ";";
		std::cerr << "move_record 3rd: [" << delete_query << "]" << std::endl;

		mysql_ping(_con);
		if(mysql_query(_con,delete_query.c_str()))
		{
			retry++;
			std::cerr << "DBA:remove_record(2) - Q[" << delete_query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
			::poll(0,0,200);
			continue;
		}
		else retry=0;
	}
	while(retry);

	return true;
}



bool DB_Access::remove_record(std::string key, std::string value, std::string tbl)
{
	if(!_con)
        {
                std::cerr << "DBA:remove_record() - invalid connection..exit" << std::endl;
                return false;
        }

	MY_QUARD(&_lock);

	//DELETE FROM `trade_data`.`orders_queue` WHERE `orderId`='230' and`id`='0';
	
	std::string query = "DELETE FROM " + _db_name + "." + tbl + " WHERE " + key + "=" + value + ";";
	//std::cerr << "RMQ:[" << query << "]" << std::endl;

	mysql_ping(_con);
	if(mysql_query(_con,query.c_str()))
	{
		std::cerr << "DBA:remove_record() - Q[" << query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}
	return true;
}


bool DB_Access::update_record(string setkey, string setvalue, string lookupkey, string lookupvalue, string tbl)
{
	if(!_con)
        {
                std::cerr << "DBA:remove_record() - invalid connection..exit" << std::endl;
                return false;
        }

	MY_QUARD(&_lock);
	//UPDATE `trade_data`.`pending_orders` SET `id`=190 WHERE `orderId`='0';
	std::string query = "UPDATE " + _db_name + "." + tbl + " SET " + setkey + "=" + setvalue 
		+ " WHERE " + lookupkey + "=" + lookupvalue + ";";
        //std::cerr << "RMQ:[" << query << "]" << std::endl;

	mysql_ping(_con);
        if(mysql_query(_con,query.c_str()))
        {
                std::cerr << "DBA:remove_record() - Q[" << query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
                return false;
        }
        return true;
}

bool DB_Access::get_record(string lookupkey, string lookupvalue, vector<pair<string,string> > &result, string tbl)
{

	if(!_con)
        {
                std::cerr << "DBA:get_rX() - invalid connection..exit" << std::endl;
                return false;
        }

	MYSQL_ROW row;
	MYSQL_FIELD *field;
	std::string query;
	query = "SELECT * FROM " + _db_name + "." + tbl + " WHERE " + lookupkey + " = " + lookupvalue + " LIMIT 1";

	MY_QUARD(&_lock);
	
	int retry = 5;
	
	do
	{
		cerr << "Q IS [" << query << "]" << endl;
		mysql_ping(_con);
		if(mysql_real_query(_con,query.c_str(),query.length()))
		{
			retry--;
			std::cerr << "DBA:get_rX() - Q[" << query << "] failed with error: [" << mysql_error(_con) << "], retry" << retry << std::endl;
			//return false;
			poll(0,0,200);
			continue;
		}
		MYSQL_RES* res = mysql_store_result(_con);
		if(!res)
		{
			retry--;
			std::cerr << "DBA:get_rX() - DBERROR: [" << mysql_error(_con) << "], errno [" 
					<< mysql_errno(_con) << "], retry = " << retry << std::endl;
			//return false;
			poll(0,0,200);
			continue;
		}
		int nf = mysql_num_fields(res);
		//std::cerr << "DBA::get_rX() - received #" << nf << std::endl;
		row = mysql_fetch_row(res);
		if(row) {
			for(int i = 0; i < nf; i++)
			{
				//std::cerr << (row[i] ? row[i] : "NULL") << " ";
				field = mysql_fetch_field_direct(res,i);
				//std::cerr << "Key=[" << field->name << "], value=[" << (row[i] ? row[i] : "NULL") << "]" << std::endl;

				std::pair<std::string,std::string> p(field->name,(row[i] ? row[i] : "NULL"));
				result.push_back(p);
			}
			//std::cerr << std::endl;
			mysql_free_result(res);
			retry=0;
		}
		else {
			std::cerr << "DBA::get_rX() - Retival failed.........retry=(" << retry << ")" << std::endl;
			poll(0,0,500);
			mysql_free_result(res);
			retry--;
		}
	}
	while(retry);
	return true;
	
}


bool DB_Access::get_records(std::vector<std::pair<std::string,std::string> > &out, std::string id,std::string tbl)
{
	if(!_con)
	{
		std::cerr << "DBA:get_r() - invalid connection..exit" << std::endl;
		return false;
	}
	std::string query;
	if(id == "*")
		query = "SELECT * FROM " + _db_name + "." + tbl + ";";
	else
		query = "SELECT * FROM " + _db_name + "." + tbl + " WHERE id = " + id + ";";

	//std::cerr << "DBA:get_r(q=" << query << ")" << std::endl;
	//
	MY_QUARD(&_lock);

	mysql_ping(_con);
	if(mysql_query(_con,query.c_str()))
	{
		std::cerr << "DBA:get_r() - Q[" << query << "] failed with error: [" << mysql_error(_con) << "]" << std::endl;
		return false;
	}

	MYSQL_RES* res = mysql_store_result(_con);
	if(!res)
	{
		std::cerr << "DBA:get_r() - DBERROR:" << mysql_error(_con) << std::endl;
		return false;
	}
	int nf = mysql_num_fields(res);
	//std::cerr << "DBA::get_r() - received #" << nf << std::endl;

	MYSQL_ROW row;
	MYSQL_FIELD *field;

	while((row = mysql_fetch_row(res)))
	{
		for(int i = 0; i < nf; i++)
		{
			//std::cerr << (row[i] ? row[i] : "NULL") << " ";
			field = mysql_fetch_field_direct(res,i);
			std::cerr << "Key=[" << field->name << "], value=[" << (row[i] ? row[i] : "NULL") << "]" << std::endl;

			std::pair<std::string,std::string> p(field->name,(row[i] ? row[i] : "NULL"));
			out.push_back(p);
		}
		std::cerr << std::endl;
	}
	mysql_free_result(res);
	return true;
}


bool DB_Access::get_orders_record(std::string tbl)
{
	return true;
}



bool DB_Access::add_record_stock(
                                std::string time,
                                std::string symbol,
                                std::string tr_type, // "trade" or "quote"
                                double tr_price,
                                int tr_volume,
                                int cum_volume,
                                double bid_price,
                                int bid_size,
                                double ask_price,
                                int ask_size,
                                std::string tbl
                        )

{
	if(!_con)
	{
		std::cerr << "DBA:get_r_stock() - invalid connection..exit" << std::endl;
		return false;
	}
	char values[256];
	sprintf(values,"'%s','%s','%s',%.2f,%d,%d,%.2f,%d,%.2f,%d",

	time.c_str(), symbol.c_str(),tr_type.c_str(),tr_price,tr_volume,cum_volume,bid_price,bid_size,ask_price,ask_size);

	std::string query = "INSERT INTO " + 
		tbl + 
		"(TIMESTAMP,SYMBOL,TR_TYPE,TRADE_PRICE,TRADE_VOLUME,CUM_VOLUME,BID_PRICE,BID_SIZE,ASK_PRICE,ASK_SIZE) values (" +
		values + ");";


		//std::cerr << "DBA::r_stock()[" << std::endl << query << std::endl << "]" << std::endl;

		mysql_ping(_con);
		if(mysql_query(_con, query.c_str()))
		{
			std::cerr << "DBA::r_stock() - Failed to update table: " << tbl << " " << std::endl;
			std::cerr << "DBA::r_stock(ERROR) - " << mysql_error(_con) << std::endl;
			return false;
		}



	return true;	
}

int DB_Access::get_nrecords(std::string tbl) const 
{
	if(!_con)
	{
		std::cerr << "DBA:nr() - invalid connection..exit" << std::endl;
		return false;
	}
	std::string query = "SELECT COUNT(*) FROM " + tbl + ";";
	mysql_ping(_con);
	if(mysql_query(_con, query.c_str()))
	{
		std::cerr << "DBA:get_nq() - failed to retrieve # of records" << std::endl;
		return -1;
	}
	MYSQL_RES* res = mysql_store_result(_con);
	if(!res)
	{
		std::cerr << "DBA:nr() - DBERROR:" << mysql_error(_con) << std::endl;
		return false;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	int nf = atoi(row[0]);
	std::cerr << "DBA::nr() - received #" << nf << std::endl;
	mysql_free_result(res);
	return nf;
}

bool DB_Access::place_orders(int id,
                          string symbol,
                          string secType,
                          string multiplier,
                          string currency,
                          long clientId,
                          string action,
                          long totalQuantity,
                          string orderType,
                          string tif,
                          string transmit,
                          string goodAfterTime,
                          string goodTillDate,
                          double lmtPrice,
                          int minQty,
                          long cancelInterval,
                          long nEvents,
                          long tid)
{
	if(!_con)
        {
                std::cerr << "DBA::place_order() - invalid connection..exit" << std::endl;
                return false;
        }
	string tbl = "orders_queue";

 	// INSERT INTO `trade_data`.`orders_queue` (id, `symbol`, `secType`, `multiplier`, `currency`, `clientId`, `action`, `totalQuantity`, `orderType`, `tif`, `transmit`, `goodAfterTime`, `goodTillDate`, `lmtPrice`, `minQty`, `cancelInterval`, `nEvents`, ticker_id) VALUES (257, 'CNSI', 'STK', 1, 'USD', 0, 'BUY', 4, 'LMT', 'GTC', 'TRUE', '20160112 09:59:00', '20160112 15:59:00', 40.00 , 2, 30, 2 , 12);


	char values[2048];
	sprintf(values,"%d,'%s','%s','%s','%s',%ld,'%s',%ld,'%s','%s','%s','%s','%s',%.2f,%d,%ld,%ld,%ld",
			id,
			symbol.c_str(),
			secType.c_str(),
			multiplier.c_str(),
			currency.c_str(),
			clientId,
			action.c_str(),
			totalQuantity,
			orderType.c_str(),
			tif.c_str(),
			transmit.c_str(),
			goodAfterTime.c_str(),
			goodTillDate.c_str(),
			lmtPrice,
			minQty,
			cancelInterval,
			nEvents,
			tid);

	std::string query = "INSERT INTO " + _db_name + "." + tbl 
		+ 
		"(id, `symbol`, `secType`, `multiplier`, `currency`, `clientId`, `action`, `totalQuantity`, `orderType`, `tif`, `transmit`, `goodAfterTime`, `goodTillDate`, `lmtPrice`, `minQty`, `cancelInterval`, `nEvents`, ticker_id) VALUES (" +
		values + ");";


	std::cerr << "DBA::place_order()[" << std::endl << query << std::endl << "]" << std::endl;

	int retry = 3;
	do
	{
		mysql_ping(_con);
		if(mysql_query(_con, query.c_str()))
		{	
			std::cerr << "DBA::place_order() - Failed to insert a new record: " << tbl << " " << std::endl;
			std::cerr << "DBA::place_order() - " << mysql_error(_con) << std::endl;
			poll(0,0,200);
			--retry;
			continue;
		}
		else
		retry=0;
	}
	while(retry);
	return true;	
	
}


