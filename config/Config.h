#ifndef _IB_CONFIG_H_
#define _IB_CONFIG_H_

#include <libini.h>
#include <string>
#include <vector>
#include <map>
#include <Contract.h>

const int max_key_value = 128;

class Config
{
	ini_fd_t	_ini_FD;
	std::string 	_ini_file;

public:
	struct SData
	{
		int		_id;
		std::string	_symbol;
		Contract	_contract;
		std::string 	_wts;
		std::string 	_command;
		int		_bar_size;
		int		_useRTH;
		std::string	_endDateTime;
		std::string	_durationStr;
		std::string	_barSizeSetting;
		int		_formatDate;
		std::string	_tickType;
		int		_client;
		int		_putID;
		int		_callID;


		// meas counters pers event
		size_t		_n_put_bid;
		size_t		_n_put_ask;
		size_t		_n_put_last; // trade for ph1
		size_t		_n_call_bid;
		size_t		_n_call_ask;
		size_t		_n_call_last; // trade for ph1
		size_t		_n_stk_ask;
		size_t		_n_stk_bid;
		size_t		_n_stk_last; // trade for ph1
		size_t		_n_other;
		size_t		_n_events; 
		size_t		_order_id;
		long 		_cancel_tid;
		SData()
		:_n_put_bid(0),
		 _n_put_ask(0),
		 _n_put_last(0),
		 _n_call_bid(0),
		 _n_call_ask(0),
		 _n_call_last(0),
		 _n_stk_ask(0),
		 _n_stk_bid(0),
		 _n_stk_last(0),
		 _n_other(0),
		 _n_events(0),
		 _order_id(0),
		 _cancel_tid(0)
		{
		}
		~SData () {}
		void dump();
	};


	class ExecCallback
	{
	public:
		ExecCallback() {}
		virtual ~ExecCallback() {}
		virtual bool handle_state(std::string node_name,SData sd) = 0;
	};

	Config();
	~Config(); 

	void dump();

	void close();


	bool init(std::string fl);

	bool getFirst(SData& sd); 

	bool getNext(SData& sd);

	bool rewind();

	bool getCurrent(SData& sd);

	bool getByName(SData& sd, std::string s);

	bool runFlow(ExecCallback* callback);

	bool getById(SData& sd, int id);

	bool getByClient(std::vector<SData>& sd_list, int cid);

private:
	std::vector<std::string> 			_commands;
	std::multimap<std::string,SData>		_info;
	std::multimap<std::string,SData>::iterator 	_curr_pos;

	bool locateKey(ini_fd_t fd,const char* key, double& val);
	bool locateKey(ini_fd_t fd,const char* key, char* val);
};



#endif
