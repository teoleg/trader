#include <libini.h>
#include <string>
#include <iostream>
#include <vector>
#include <map>

#include <Contract.h>

#include <Config.h>

Config::Config() {}
Config::~Config() 
{
	//ini_close(_ini_FD);
}

void Config::close()
{
	//std::cerr << "CLOSE FD" << std::endl;
	ini_close(_ini_FD);
}

void Config::SData::dump()
{
	if(_command.size())
	std::cerr << "   COMMAND    : " << _command << std::endl;
	//if(_id)
	//std::cerr << "   TickerId       : " << _id << std::endl;
	if(_client)
	std::cerr << "   client id      : " << _client << std::endl;
	if(_putID)
	std::cerr << "   Put ID         : " << _putID << std::endl;
	if(_callID)
	std::cerr << "   Call ID        : " << _callID << std::endl;
	if(_contract.symbol.size())
	std::cerr << "   Symbol         : " << _contract.symbol << std::endl;
	if(_contract.localSymbol.size())
	std::cerr << "   localSymbol    : " << _contract.localSymbol << std::endl;
	if(_contract.secType.size())
	std::cerr << "   secType        : " << _contract.secType << std::endl;
	if(_contract.exchange.size())
	std::cerr << "   exchange       : " << _contract.exchange << std::endl;
	if(_contract.currency.size())
	std::cerr << "   currency       : " << _contract.currency << std::endl;
	if(_contract.strike)
	std::cerr << "   strike         : " << _contract.strike << std::endl;
	if(_contract.multiplier.size())
	std::cerr << "   multiplier     : " << _contract.multiplier << std::endl;
	if(_contract.expiry.size())
	std::cerr << "   expiry         : " << _contract.expiry << std::endl;
	if(_contract.right.size())
	std::cerr << "   right          : " << _contract.right << std::endl;
	if(_wts.size())
	std::cerr << "   what to show   : " << _wts << std::endl;
	if(_bar_size)
	std::cerr << "   bar size       : " << _bar_size << std::endl;
	if(_useRTH)
	std::cerr << "   use RTH        : " << _useRTH << std::endl;
	if(_endDateTime.size())
	std::cerr << "   endDateTime    : " << _endDateTime << std::endl;
	if(_durationStr.size())
	std::cerr << "   durationStr    : " << _durationStr << std::endl;
	if(_barSizeSetting.size())
	std::cerr << "   barSizeSetting : " << _barSizeSetting << std::endl;
	if(_formatDate)
	std::cerr << "   formatDate     : " << _formatDate << std::endl;
	if(_tickType.size())
	std::cerr << "   tickType       : " << _tickType << std::endl;
}


void Config::dump()
{
	for(std::multimap<std::string,SData>::iterator it = _info.begin();
			it != _info.end();
			it++)
	{
		std::cerr << "====================================" << std::endl;
		std::cerr << "COMMAND: " << it->first << std::endl;
			it->second.dump();
		std::cerr << "====================================" << std::endl;
	}
}


bool Config::init(std::string fl)
{
	std::cerr << "Open conifuration file: " << fl << std::endl;
	_ini_FD = ini_open(fl.c_str(),"w",";");
	if(!_ini_FD)
	{
		std::cerr << "Faield to open config file" << std::endl;
		return false;
	}

	// find generic section

	// find exection flow section
	if(ini_locateHeading (_ini_FD, "Request") < 0)
	{
		std::cerr << "Failed to locate 'REquest' section in " << _ini_file << std::endl;
		return false;
	}

	if(ini_locateKey(_ini_FD,"Set") < 0)
	{
		std::cerr << "Failed to locate key 'Set' " << std::endl;
		return false;
	}

	char str[max_key_value];
               if(!ini_readString(_ini_FD,str,sizeof(str)))
	{
		std::cerr << "Failed to read the value of 'Set' " << std::endl;
		return false;
	}
	std::cerr << "[Request]: Set is [" << str << "]" << std::endl;
	std::string sp = str, sv;
	size_t begin = 0, end = 0;

	for(std::string::iterator it = sp.begin(); it != sp.end(); it++)
	{
		if(*it == ',')
		{
			sv = sp.substr(begin, (end - begin));
			end++; begin = end;
			_commands.push_back(sv);
		}
		else if((it + 1) == sp.end())
		{
			sv = sp.substr(begin, (end - begin) + 1);
			_commands.push_back(sv);
		}
		else end++;
	}

	// get parameters
	for(size_t i = 0; i < _commands.size(); i++)
	{
		Contract tmp_ctr;
		double tmpd = 0;
		SData s;

		if(ini_locateHeading(_ini_FD,_commands[i].c_str()) < 0)
		{
			std::cerr << "Failed to locate section " << _commands[i] << std::endl;
			return false;
		}

		if(locateKey(_ini_FD,"Command",str))
		 	s._command = str;	

		if(locateKey(_ini_FD,"ID",str))
		 	s._id = atoi(str);	

		if(locateKey(_ini_FD,"secType",str)) 
		 	tmp_ctr.secType = str;

		if(locateKey(_ini_FD,"symbol",str)) 
		 	tmp_ctr.symbol = str;

		if(locateKey(_ini_FD,"localSymbol",str))
		 	tmp_ctr.localSymbol = str;	
		
		if(locateKey(_ini_FD,"exchange",str))
		 	tmp_ctr.exchange = str;	

		if(locateKey(_ini_FD,"currency",str))
		 	tmp_ctr.currency = str;	

		if(locateKey(_ini_FD,"strike",tmpd))
		 	tmp_ctr.strike = tmpd;	

		if(locateKey(_ini_FD,"multiplier",str))
		 	tmp_ctr.multiplier = str;	

		if(locateKey(_ini_FD,"expiry",str))
		 	tmp_ctr.expiry = str;	

		if(locateKey(_ini_FD,"right",str))
		 	tmp_ctr.right = str;	

		if(locateKey(_ini_FD,"whattoshow",str))
		 	s._wts = str;	

		if(locateKey(_ini_FD,"bar_size",str))
		 	s._bar_size = atoi(str);	

		if(locateKey(_ini_FD,"useRTH",str))
		 	s._useRTH = atoi(str);

		if(locateKey(_ini_FD,"endDateTime",str))
		 	s._endDateTime = str;

		if(locateKey(_ini_FD,"durationStr",str))
		 	s._durationStr = str;

		if(locateKey(_ini_FD,"barSizeSetting",str))
		 	s._barSizeSetting = str;

		if(locateKey(_ini_FD,"formatDate",str))
		 	s._formatDate = atoi(str);

		if(locateKey(_ini_FD,"tickType",str))
			s._tickType = str;

		if(locateKey(_ini_FD,"client",str))
			s._client = atoi(str);

		if(locateKey(_ini_FD,"putID",str))
			s._putID = atoi(str);

		if(locateKey(_ini_FD,"callID",str))
			s._callID = atoi(str);

		// populate contract structure
		s._contract = tmp_ctr;
		_info.insert(std::make_pair(_commands[i],s));
	}
	return true;
}


bool Config::getFirst(SData& sd) 
{
	std::multimap<std::string,SData>::iterator it = _info.find(_commands[0]);	
	if(it == _info.end())
		return false;
	sd = it->second;
	_curr_pos = it;
	return true;
}

bool Config::getNext(SData& sd)
{
	std::multimap<std::string,SData>::iterator it = _curr_pos;
	it++;
	if(it == _info.end())
		return false;
	sd = it->second;
	_curr_pos = it;
	return true;
}

bool Config::rewind()
{
	_curr_pos = _info.begin();
	return true;
}

bool Config::getCurrent(SData& sd)
{
	if(_curr_pos == _info.end())
		return false;
	sd = _curr_pos->second;
	return true;
}

bool Config::getByName(SData& sd, std::string s)
{
	std::multimap<std::string,SData>::iterator it = _info.find(s);
	if(it == _info.end())
		return false;
	sd = it->second;
	return true;
}



bool Config::getById(SData& sd, int id)
{
	if(id < 0) return false;
	for(std::multimap<std::string,SData>::iterator it = _info.begin();
	    it != _info.end();
	    it++)
	{
		//std::cerr << "Travering... ID=" << it->second._id << " where id=" << id << std::endl;
		if(it->second._id == id)
		{
			//std::cerr << "Found it... " << std::endl;
			sd = it->second;
			return true;
		}
	}
	return false;
}

bool Config::getByClient(std::vector<SData>& sd_list, int cid)
{
	if(cid < 0) return 0;
	for(std::multimap<std::string,SData>::iterator it = _info.begin();
	    it != _info.end();
	    it++)
	{
		//std::cerr << "Traversing... CID=" << it->second._client << " where id=" << cid << std::endl;
		if(it->second._client == cid)
		{
			//std::cerr << "Found it... " << std::endl;
			sd_list.push_back(it->second);
		}
	}
	return sd_list.size() ? true : false;

}


bool Config::locateKey(ini_fd_t fd,const char* key, double& val)
{
	if(!key) return false;
	if(!fd) return false;
	if(ini_locateKey(fd,key) < 0)
	{
		//std::cerr << "Failed to locate [" << key << "]" << std::endl;
		return false;
	}	

	if(ini_readDouble(_ini_FD,&val))
	{
		//std::cerr << "Failed to read double value of '" << val << "'" << std::endl;
		return false;
	}
	return true;
}

bool Config::locateKey(ini_fd_t fd,const char* key, char* val)
{
	if(!key) return false;
	if(!fd) return false;
	if(ini_locateKey(fd,key) < 0)
	{
		//std::cerr << "Failed to locate [" << key << "]" << std::endl;
		return false;
	}	

	val[0]='\0';
	if(!ini_readString(_ini_FD,val,max_key_value))
	{
		//std::cerr << "Failed to read value of '" << val << "'" << std::endl;
		return false;
	}
	return true;
}


bool Config::runFlow(Config::ExecCallback* callback)
{
	if(!callback) return false;
	for(std::multimap<std::string,SData>::iterator it = _info.begin();
		it != _info.end();
		it++)
	{
		if(!callback->handle_state(it->first,it->second))
			return false;	
	}
	return true;
}
