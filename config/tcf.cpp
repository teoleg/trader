#include <Config.h>
#include <iostream>


class Test : public Config::ExecCallback
{
public:
	Test():Config::ExecCallback() {}
	bool handle_state(std::string name, Config::SData sd)
	{
		std::cerr << "Command: " << name << std::endl;
		return true;
	}
};

int main()
{
	Config cf;	
	cf.init("./IBC.ini");
	cf.dump();


	std::cerr << "Get first element" << std::endl;
	Config::SData s;
	if(cf.getFirst(s))
	    s.dump();
	std::cerr << "--- End of test ---" << std::endl;
	


	std::cerr << "Get next element" << std::endl;
	Config::SData s2,s3;
	if(cf.getNext(s2))
	    s2.dump();
	std::cerr << "Get one more... " << std::endl;
	if(cf.getNext(s3))
	    s3.dump();
	std::cerr << "--- check integrity ---" << std::endl;
	    s.dump();
	std::cerr << "--- End of test ---" << std::endl;



	std::cerr << "--- Test rewind ---" << std::endl;
	Config::SData s4;
	if(cf.rewind())
	   if(cf.getNext(s4))
	        s4.dump();
	std::cerr << "--- End of test ---" << std::endl;



	std::cerr << "Test getByName ----" << std::endl;
	Config::SData s5,s6;
	if(cf.getByName(s5,"RHD"))
		s5.dump();
	std::cerr << "Get one more... " << std::endl;
	if(cf.getByName(s6,"RRTB"))
		s6.dump();
	std::cerr << "--- End of test ---" << std::endl;


	std::cerr << "Test callback---" << std::endl;

	Test* tst = new Test;

	cf.runFlow(tst);
	std::cerr << "--- End of test ---" << std::endl;

	std::cerr << "--- Test get by ID -----" << std::endl;
	Config::SData s7, s8;
	if(cf.getById(s7,1))
		s7.dump();
	if(cf.getById(s8,7))
		s8.dump();

	std::cerr << " --- End of test ---" << std::endl;

	std::cerr << "-- Test client list --- " << std::endl;

	std::vector<Config::SData> sdl1;

	if(cf.getByClient(sdl1,1))
	{
		if(sdl1.size())
		{
			for(size_t i=0; i < sdl1.size(); i++)
			{
				std::cerr << "-----------" << std::endl;
				sdl1[i].dump();
			}
		}
	}


	std::cerr << " --- End of test ---" << std::endl;

	cf.close();
}

