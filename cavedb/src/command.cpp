#include "command.h"
/// namespace lyramilk::cave
namespace lyramilk{ namespace cave
{

	cmdchanneldata::cmdchanneldata()
	{
		loginseq = 0;
		isreadonly = true;
	}

	cmdchanneldata::~cmdchanneldata()
	{
		
	}

	void cmdchanneldata::set_requirepass(const lyramilk::data::string& requirepass)
	{
		this->requirepass = requirepass;
		srand(time(nullptr));
		this->loginseq = rand();
	}

	void cmdchanneldata::set_readonly(bool isreadonly)
	{
		this->isreadonly = isreadonly;
	}
}}