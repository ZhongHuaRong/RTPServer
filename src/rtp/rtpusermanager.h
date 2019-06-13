
#pragma once
#include "rtpsession.h"
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include "jrtplib3/rtpsourcedata.h"
#include "jrtplib3/rtpipv4address.h"
#include "jrtplib3/rtpsessionparams.h"
#include "jrtplib3/rtpudpv4transmitter.h"
#include "../core/logger.h"
#include "../serverengine.h"
#include "queue.h"

namespace rtpserver {

struct User{
	std::string name;
	//1绑定1,2绑定2
	//ssrc1对应port1和videosession
	//ssrc2对应port2和audiosession
	uint32_t ssrc{0};
	uint32_t ssrc2{0};
	uint32_t ip;
	//为了方便，rtp和rtcp端口统一
	uint16_t rtp_port1;
	uint16_t rtcp_port1;
	uint16_t rtp_port2;
	uint16_t rtcp_port2;
	Session * session{nullptr};
};

class RTPQueue;
class RTCPQueue;

struct Room{
public:
	using RoomUser = std::shared_ptr<User>;
	std::map<std::string,RoomUser> sender;
	std::map<std::string,RoomUser> user;
	std::string name;
	std::recursive_mutex sender_mutex;
	std::recursive_mutex user_mutex;
	RTPQueue *rtp_queue;
	RTCPQueue *rtcp_queue;
	using RTPPKT = AbstractQueue<RTPPacket>::pointer;
	using RTCPPKT = AbstractQueue<RTCPPacket>::pointer;
	
	Room();
	~Room();
	
	inline void clear() noexcept {
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		std::lock_guard<std::recursive_mutex> lk1(user_mutex);
		sender.clear();
		user.clear();
	}
	
	inline uint64_t size() noexcept {
		return sender.size() + user.size();
	}
	
	inline uint64_t sender_size() noexcept {
		return sender.size();
	}
	
	inline bool find_sender(std::string name,RoomUser&ptr) noexcept{
		try {
			std::lock_guard<std::recursive_mutex> lk(sender_mutex);
			auto& p = sender.at(name);
			ptr = p;
			return true;
		} catch (const std::out_of_range&) {
			return false;
		}
	}
	
	inline bool find_user(std::string name,RoomUser&ptr) noexcept{
		try {
			std::lock_guard<std::recursive_mutex> lk(user_mutex);
			auto& p = user.at(name);
			ptr = p;
			return true;
		} catch (const std::out_of_range&) {
			return false;
		}
	}
	
	/**
	 * @brief insert_user
	 * 添加用户，is_sender标志着这个用户是发送者
	 * 添加后返回该对象
	 */
	inline RoomUser user_join(std::string name,bool is_sender,
								const uint32_t& ssrc,
								const uint32_t&ip,
								const uint16_t&rtp_port,const uint16_t&rtcp_port) noexcept{
		RoomUser _user;
		//添加的用户是发送者
		if(is_sender){
			//先在查找普通用户里面查找，如果存在则移动到发送者
			auto ret = find_user(name,_user);
			if(ret == true){
				_erase_user(name);
				set_user_information(_user,ssrc,ip,rtp_port,rtcp_port);
				_user_change_to_sender(name,_user);
				_insert_sender(_user,name);
			}
			else {
				//去发送者里面找
				ret = find_sender(name,_user);
				if(ret == true){
					set_user_information(_user,ssrc,ip,rtp_port,rtcp_port);
				}
				else {
					//两个都找不到，需要new一个
					auto __user = std::make_shared<User>();
					__user->name = name;
					set_user_information(__user,ssrc,ip,rtp_port,rtcp_port);
					_user_change_to_sender(name,__user);
					_insert_sender(__user,name);
					return __user;
				}
			}
		}
		//添加的用户是接受者
		else {
			//先在发送者里面查找，可能是发送者转为接收者了
			auto ret = find_sender(name,_user);
			if(ret == true){
				_erase_sender(name);
				set_user_information(_user,ssrc,ip,rtp_port,rtcp_port);
				_sender_change_to_user(name,_user);
				_insert_user(_user,name);
			}
			else {
				ret = find_user(name,_user);
				if(ret == true){
					set_user_information(_user,ssrc,ip,rtp_port,rtcp_port);
				} else {
					auto __user = std::make_shared<User>();
					__user->name = name;
					set_user_information(__user,ssrc,ip,rtp_port,rtcp_port);
					_insert_user(__user,name);
					return __user;
				}
			}
		}
		return _user;
	}
	
	inline void user_exit(std::string name,
						  const uint32_t& ssrc) noexcept{
		RoomUser _user;
		auto ret = find_user(name,_user);
		if(ret == true){
			_user_exit(_user,name,ssrc);
		}
		else {
			ret = find_sender(name,_user);
			if(ret == true){
				_user_exit(_user,name,ssrc);
			}
			else {
				//没找着，记录
				rtpserver::Logger::Warning_app(rtpserver::MessageNum::Rtcp_remove_abnormal,
											   "Room::user_exit",
											   name.c_str());
			}
		}
	}
	
	inline void send_packet(std::shared_ptr<RTPPacket> pkt) noexcept{
		size_t len;
		void *data = pkt->srcdat->SDES_GetName(&len);
		if(len == 0)
			return;
		std::string name(static_cast<char*>(data),len);
		RoomUser _user;
		auto ret = find_sender(name,_user);
		if(ret == true){
			RTPSession *session;
			switch(pkt->rtppack->GetPayloadType()){
			case 96:
			case 97:
			case 98:
				session = &_user->session->video;
//				session =  &RegisterEngine::Get_Engine(ENGINENAME)->session.video;
				session->SetDefaultPayloadType(pkt->rtppack->GetPayloadType());
				break;
			case 99:
				session = &_user->session->audio;
				session->SetDefaultPayloadType(pkt->rtppack->GetPayloadType());
				break;
			default:
				return;
			}
			
			int ret;
			if(pkt->rtppack->GetExtensionLength() == 0){
				//没有额外信息
				ret = session->SendPacket(pkt->rtppack->GetPayloadData(),pkt->rtppack->GetPayloadLength());
			} else {
				ret = session->SendPacketEx(pkt->rtppack->GetPayloadData(),pkt->rtppack->GetPayloadLength(),
											pkt->rtppack->GetExtensionID(),pkt->rtppack->GetExtensionData(),
											pkt->rtppack->GetExtensionLength());
			}
			
			if( ret < 0 ){
				constexpr char api[] = "Room::send_packet";
				rtpserver::Logger::Error_app(rtpserver::MessageNum::Rtp_send_packet_failed,
										api);
				rtpserver::Logger::Error_rtp(ret,api);
			}
		}
	}
	
	void push_rtp_packet(RTPPacket *packet) noexcept;
	
	void push_rtcp_packet(RTCPPacket *packet) noexcept;
	
protected:
	inline void set_user_information(RoomUser& room_user,const uint32_t& ssrc,
									const uint32_t& ip,const uint16_t& rtp_port,const uint16_t& rtcp_port) noexcept{
		//端口和ｉｐ更换后的判断和更新
		if(room_user->ssrc == ssrc){
			if(room_user->ip != ip || room_user->rtp_port1 != rtp_port || room_user->rtcp_port1 != rtcp_port){
				_sender_video_delete_destination(room_user->ip,room_user->rtp_port1,room_user->rtcp_port1);
				room_user->ip = ip;
				room_user->rtp_port1 = rtp_port;
				room_user->rtcp_port1 = rtcp_port;
				_sender_video_add_destination(room_user->name,room_user->ip,room_user->rtp_port1,room_user->rtcp_port1);
			}
			return;
		}
		else if(room_user->ssrc2 == ssrc){
			if(room_user->ip != ip || room_user->rtp_port2 != rtp_port || room_user->rtcp_port2 != rtcp_port){
				_sender_audio_delete_destination(room_user->ip,room_user->rtp_port2,room_user->rtcp_port2);
				room_user->ip = ip;
				room_user->rtp_port2 = rtp_port;
				room_user->rtcp_port2 = rtcp_port;
				_sender_audio_add_destination(room_user->name,room_user->ip,room_user->rtp_port2,room_user->rtcp_port2);
			}
			return;
		}
		
		if(room_user->ssrc == 0){
			room_user->ssrc = ssrc;
			room_user->ip = ip;
			room_user->rtp_port1 = rtp_port;
			room_user->rtcp_port1 = rtcp_port;
			_sender_video_add_destination(room_user->name,room_user->ip,room_user->rtp_port1,room_user->rtcp_port1);
			log(room_user,"Room::set_user_information",MessageNum::Rtcp_set_user_information);
		}
		else if(room_user->ssrc2 == 0){
			room_user->ssrc2 = ssrc;
			room_user->ip = ip;
			room_user->rtp_port2 = rtp_port;
			room_user->rtcp_port2 = rtcp_port;
			_sender_audio_add_destination(room_user->name,room_user->ip,room_user->rtp_port2,room_user->rtcp_port2);
			log(room_user,"Room::set_user_information",MessageNum::Rtcp_set_user_information);
		}
	}
	
private:
	inline void _insert_sender(RoomUser& room_user,std::string& name) noexcept{
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		sender.insert( std::pair<std::string,RoomUser>(name,room_user));
		log(room_user,"Room::insert_sender",MessageNum::Rtcp_insert_user_successful);
	}
	
	inline void _erase_sender(std::string& name) noexcept{
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		sender.erase(name);
	}
	
	inline void _insert_user(RoomUser& room_user,std::string& name) noexcept{
		std::lock_guard<std::recursive_mutex> lk(user_mutex);
		user.insert( std::pair<std::string,RoomUser>(name,room_user));
		log(room_user,"Room::_insert_user",MessageNum::Rtcp_insert_user_successful);
	}
	
	inline void _erase_user(std::string& name) noexcept{
		std::lock_guard<std::recursive_mutex> lk(user_mutex);
		user.erase(name);
	}
	
	inline void _user_change_to_sender(std::string& name,
									   RoomUser& room_user) noexcept{
		if(room_user->session == nullptr){
			auto port = RegisterEngine::Get_Engine(ENGINENAME)->port_base;
			int ret;
			room_user->session = Session::CreateSession(port,port + 2,
														1.0 / 15.0, 1.0 / 8000.0,
														ret);
			if(ret != 0) {
				Logger::Error_rtp(ret,"create");
				return;
			}
		}
		else {
			room_user->session->audio.ClearDestinations();
			room_user->session->video.ClearDestinations();
		}
		
		room_user->session->video.SetDefaultMark(false);
		room_user->session->audio.SetDefaultMark(false);
		room_user->session->video.SetDefaultTimestampIncrement(1);
		room_user->session->audio.SetDefaultTimestampIncrement(1);
		room_user->session->video.SetLocalName(name.c_str(),name.size());
		room_user->session->audio.SetLocalName(name.c_str(),name.size());
		room_user->session->video.SetNameInterval(1);
		room_user->session->audio.SetNameInterval(1);
		room_user->session->video.SetMaximumPacketSize(65535u);
		room_user->session->audio.SetMaximumPacketSize(65535u);
		
		{
			std::lock_guard<std::recursive_mutex> lk(user_mutex);
			for( auto i = user.begin();i != user.end(); ++i){
				if(i->second->ssrc != 0)
					room_user->session->video.AddDestination(jrtplib::RTPIPv4Address(
																 i->second->ip,i->second->rtp_port1,i->second->rtcp_port1));
				if(i->second->ssrc2 != 0)
					room_user->session->audio.AddDestination(jrtplib::RTPIPv4Address(
																 i->second->ip,i->second->rtp_port2,i->second->rtcp_port2));
			}
		}
		{
			std::lock_guard<std::recursive_mutex> lk(sender_mutex);
			for( auto i = sender.begin();i != sender.end(); ++i){
				if(i->second->ssrc != 0)
					room_user->session->video.AddDestination(jrtplib::RTPIPv4Address(
																 i->second->ip,i->second->rtp_port1,i->second->rtcp_port1));
				if(i->second->ssrc2 != 0)
					room_user->session->audio.AddDestination(jrtplib::RTPIPv4Address(
																 i->second->ip,i->second->rtp_port2,i->second->rtcp_port2));
			}
		}
		log(room_user,"Room::_user_change_to_sender",MessageNum::Rtcp_mode_change);
	}
	
	inline void _sender_change_to_user(std::string& name,
									   RoomUser& room_user) noexcept {
		UNUSED(name)
		if(room_user->session != nullptr){
			Session::DestroySession(room_user->session);
			room_user->session = nullptr;
		}
		log(room_user,"Room::_sender_change_to_user",MessageNum::Rtcp_mode_change);
	}
	
	inline void _user_exit(RoomUser& room_user,
						   std::string& name,
						   const uint32_t& ssrc) noexcept{
		if(room_user->ssrc == ssrc){
			room_user->ssrc = 0;
			_sender_video_delete_destination(room_user->ip,room_user->rtp_port1,room_user->rtcp_port1);
		}
		else if(room_user->ssrc2 == ssrc){
			room_user->ssrc2 = 0;
			_sender_audio_delete_destination(room_user->ip,room_user->rtp_port2,room_user->rtcp_port2);
		}
		
		if(room_user->ssrc == 0 && room_user->ssrc2 == 0){
			log(room_user,"Room::_user_exit",MessageNum::Rtcp_remove_user_successful);
			if(room_user->session == nullptr)
				_erase_user(name);
			else
				_erase_sender(name);
		}
		else {
			//?
		}
	}
	
	inline void _sender_video_add_destination(const std::string& name,
											  const uint32_t& ip,
											  const uint16_t& rtp_port,
											  const uint16_t& rtcp_port) noexcept{
		auto ptr =  RegisterEngine::Get_Engine(ENGINENAME);
		if(ptr != nullptr){
			ptr->add_destination(ip,rtp_port,rtcp_port);
		}
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		for( auto i = sender.begin();i != sender.end(); ++i){
			if(i->second->name != name)
				i->second->session->video.AddDestination(jrtplib::RTPIPv4Address(ip,rtp_port,rtcp_port));
		}
	}
	
	inline void _sender_audio_add_destination(const std::string& name,
											  const uint32_t& ip,
											  const uint16_t& rtp_port,
											  const uint16_t& rtcp_port) noexcept{
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		for( auto i = sender.begin();i != sender.end(); ++i){
			if(i->second->name != name)
				i->second->session->audio.AddDestination(jrtplib::RTPIPv4Address(ip,rtp_port,rtcp_port));
		}
	}
	
	inline void _sender_video_delete_destination(const uint32_t& ip,
												 const uint16_t& rtp_port,
												 const uint16_t& rtcp_port) noexcept{
		auto ptr =  RegisterEngine::Get_Engine(ENGINENAME);
		if(ptr != nullptr){
			ptr->delete_destination(ip,rtp_port,rtcp_port);
		}
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		for( auto i = sender.begin();i != sender.end(); ++i){
			i->second->session->video.DeleteDestination(jrtplib::RTPIPv4Address(
															ip,rtp_port,rtcp_port));
		}
	}
	
	inline void _sender_audio_delete_destination(const uint32_t& ip,
												 const uint16_t& rtp_port,
												 const uint16_t& rtcp_port) noexcept{
		std::lock_guard<std::recursive_mutex> lk(sender_mutex);
		for( auto i = sender.begin();i != sender.end(); ++i){
			i->second->session->audio.DeleteDestination(jrtplib::RTPIPv4Address(
															ip,rtp_port,rtcp_port));
		}
	}
	
	inline void log(RoomUser& room_user,const char *api,const rtpserver::MessageNum& num) noexcept {
		char ip[16];
		const auto &i = room_user->ip;
		sprintf(ip,"%d.%d.%d.%d", (i >> 24) & 0x000000FF, (i >> 16) & 0x000000FF,
				(i >> 8) & 0x000000FF,i & 0x000000FF);
		Logger::Info_app(num,
						 api,
						 room_user->name.c_str(),
						 ip,room_user->rtp_port1,room_user->rtcp_port1,
						 ip,room_user->rtp_port2,room_user->rtcp_port2,
						 room_user->ssrc,
						 room_user->ssrc2);
	}
};

class RTPUserManager {
public:
	
	inline static RTPUserManager* GetObject() noexcept{
		if(user_manager == nullptr)
			user_manager = new RTPUserManager;
		return user_manager;
	}
	
	inline static void Destroy() noexcept{
		if( user_manager !=nullptr)
			delete  user_manager;
	}
	
	inline void deal_with_rtp_packet(RTPPacket* packet) noexcept {
		size_t len;
		void * data = packet->srcdat->SDES_GetNote(&len);
		if(len == 0){
			delete packet;
			return;
		}
		std::string name(static_cast<char*>(data),len - 1);
		list_mutex.lock();
		auto& room = room_list[name];
		list_mutex.unlock();
		if(room.get() == nullptr){
			room = std::make_shared<Room>();
			room->name = name;
		}
		room->push_rtp_packet(packet);
	}
	
	inline void deal_with_rtcp_packet(RTCPPacket* packet) noexcept {
		list_mutex.lock();
		auto& room = room_list[packet->room];
		list_mutex.unlock();
		if(room.get() == nullptr){
			room = std::make_shared<Room>();
			room->name = packet->room;
		}
		room->push_rtcp_packet(packet);
	}
private:
	static RTPUserManager * user_manager;
	std::mutex list_mutex;
	std::map<std::string,std::shared_ptr<Room>> room_list;
};


} // namespace rtpserver

