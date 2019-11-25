#include <DBAccess.h>

using namespace std;

int main()
{
	DB_Access dba;

	dba.init();

	//vector<pair<string,string> > out;

	//dba.get_records(out,"*","options_data");

	//dba.get_records(out,"*","orders_queue");

	//dba.add_record("August 15, 2013 14:59:00.000",
	//		"CNSI",
	//		"P",
	//		12.45,
	//		100000,
	//		45.12,
	//		"A",
	//		2222,"options_data");

	DB_Access::ORDERS o;
	dba.get_orders(o);

	cerr << "LEN=" << o.size() << endl;

	DB_Access::ORDERS::iterator it;
	for(it = o.begin(); it != o.end(); it++)
	{
		cerr << "ID=" << it->first << endl;
		DB_Access::PLIST::iterator pit;
		
		//DB_Access::PLIST tmp_list = it->second;
		//pit = tmp_list.find("symbol");

		string s = ((DB_Access::PLIST::iterator) ((DB_Access::PLIST)it->second).find("symbol"))->second;

		cerr << "Symbol [" << s << "]" << endl;

	}

/*
	dba.add_record_stock(
                   "August 15, 2013 14:59:00.000",
                   "C",
                   "trade", // "trade" or "quote"
                   12.22,
                   123,
                   345,
                   45.33,
                   454,
                   45.77,
                   100);


*/
	int i = dba.get_nrecords("options_data");
	std::cerr << "# records: " << i << std::endl;

	vector<pair <string, string> > r;	
	if(!dba.get_record("OrderId","99", r))
	{
		cerr << "Failed to get param" << endl;
		return -1;
	}

	for(size_t i = 0; i < r.size(); i++)
	{
		cerr << "Key=[" << r[i].first << "], value=[" << r[i].second << "]" << endl;
	}

}
