#include "serverengine.h"
#include "jrtplib3/rtpudpv4transmitter.h"
#include "jrtplib3/rtpsessionparams.h"
#include "core/logger.h"
#include "rtp/rtpusermanager.h"

namespace rtpserver{

ServerEngine::ServerEngine():
	session(nullptr)
{
	
}

ServerEngine::ServerEngine(const uint16_t &port_base):
	session(nullptr)
{
	init_engine(port_base);
}

ServerEngine::~ServerEngine()
{
	Session::DestroySession(session);
	RTPUserManager::Destroy();
}

bool ServerEngine::init_engine(const uint16_t &port_base) noexcept
{
	constexpr char api[] = "ServerEngine::init_engine";
	RTPUserManager::GetObject();
	if(session == nullptr){
		int ret;
		this->port_base = port_base;
		session = Session::CreateSession(port_base, port_base + 2,
										 1.0 / 15.0, 1.0 / 8000.0,ret);
		if(ret == -1){
			Logger::Error("-1",api);
		}
		else if( ret != 0)
			Logger::Error_rtp(ret,api);
		else {
			rtpserver::Logger::Info_app(MessageNum::Rtp_listening_port_base_success,
										api,
										"video",
										port_base);
			rtpserver::Logger::Info_app(MessageNum::Rtp_listening_port_base_success,
										api,
										"audio",
										port_base + 2);
		}
	}
	else 
		return false;
	return true;
}

int ServerEngine::exec() noexcept
{
	std::unique_lock<decltype (_event_mutex)> lk(_event_mutex);
	_event_cond_var.wait(lk,[](){
		return false;
	});
	return 0;
}

RegisterEngine RegisterEngine::Register;

}// namespace rtpserver
