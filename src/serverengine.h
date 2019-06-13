
#pragma once
#include "core/config.h"
#include "rtp/rtpsession.h"
#include "jrtplib3/rtpipv4address.h"
#include <map>
#include <condition_variable>

namespace rtpserver{

class ServerEngine
{
public:
	ServerEngine();
	
	explicit ServerEngine(const uint16_t& port_base);
	
	~ServerEngine();
	
	bool init_engine(const uint16_t& port_base) noexcept;
	
	int exec() noexcept;
	
	inline void add_destination(const uint32_t& ip,const uint16_t& port,const uint16_t& port2) noexcept {
		if(session!=nullptr)
			session->video.AddDestination(jrtplib::RTPIPv4Address(ip,port,port2));
	}
	
	inline void delete_destination(const uint32_t& ip,const uint16_t& port,const uint16_t& port2) noexcept {
		if(session!=nullptr)
			session->video.DeleteDestination(jrtplib::RTPIPv4Address(ip,port,port2));
	}
	
private:
	Session *session;
	std::mutex _event_mutex;
	std::condition_variable _event_cond_var;
	uint16_t port_base;
	friend class Room;
};

class RegisterEngine 
{
public:
	static RegisterEngine Register;
	
	inline static void Register_Engine(std::string name,ServerEngine *engine) noexcept{
		Register.list[name] = engine;
	}
	
	inline static ServerEngine * Get_Engine(const std::string& name) noexcept {
		try {
			auto& p = Register.list.at(name);
			return p;
		} catch (const std::out_of_range&) {
			return nullptr;
		}
	}
private:
	std::map<std::string,ServerEngine*> list;
};

}// namespace rtpserver
