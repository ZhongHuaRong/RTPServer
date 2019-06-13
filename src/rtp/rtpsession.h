
#pragma once
#include "../core/config.h"
#include "jrtplib3/rtpsession.h"
#include "jrtplib3/rtppacket.h"
#include "jrtplib3/rtpudpv4transmitter.h"
#include "jrtplib3/rtpsessionparams.h"
#include <memory>
#include <unistd.h>
#include "../core/logger.h"

namespace rtpserver {

struct RTPPacket{
	jrtplib::RTPSourceData *srcdat;
	std::shared_ptr<jrtplib::RTPPacket> rtppack;
};

struct RTCPPacket{
	bool is_join;
	bool is_sender;
	uint32_t SSRC;
	std::string name;
	std::string room;
	uint32_t ip;
	uint16_t rtp_port;
	uint16_t rtcp_port;
};

class RTPSession: public jrtplib::RTPSession{
public:
	RTPSession();
	
	virtual ~RTPSession() override;
protected:
	virtual void OnValidatedRTPPacket(jrtplib::RTPSourceData *srcdat, jrtplib::RTPPacket *rtppack,
									  bool isonprobation, bool *ispackethandled) override ;
	
	virtual void OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack,
									  const jrtplib::RTPTime &receivetime,
									  const jrtplib::RTPAddress *senderaddress) override;
	
	void deal_with_sdes(jrtplib::RTCPSDESPacket*,
						const uint32_t& ip,
						const uint16_t &port,
						const uint16_t &rtcp_port) noexcept;
};

struct Session{
	RTPSession video;
	RTPSession audio;
	int video_socket{0};
	int audio_socket{0};
	
	Session() = default;
	Session(const Session&) = delete;
	Session& operator = (const Session&) = delete;
	
	inline static Session * CreateSession(const uint16_t& video_port,const uint16_t& audio_port,
										  const double & video_timestamp_unit,const double & audio_timestamp_unit,
										  int &init_ret) noexcept {
		auto session = new Session();
		if(session == nullptr){
			init_ret = -1;
			return nullptr;
		}
		jrtplib::RTPSessionParams sessparams;
		jrtplib::RTPUDPv4TransmissionParams transparams;
		sessparams.SetAcceptOwnPackets(true);
		sessparams.SetOwnTimestampUnit(video_timestamp_unit);
		session->video_socket = init_socket(video_port);
		if(session->video_socket < 0){
			delete session;
			return nullptr;
		}
		transparams.SetUseExistingSockets(session->video_socket,session->video_socket);
		
		auto ret1 = session->video.Create(sessparams,&transparams);
		
		sessparams.SetOwnTimestampUnit(audio_timestamp_unit);
		session->audio_socket = init_socket(audio_port);
		if(session->audio_socket < 0){
			session->video.Destroy();
			Close_Socket(session->video_socket);
			delete session;
			return nullptr;
		}
		transparams.SetUseExistingSockets(session->audio_socket,session->audio_socket);
		
		auto ret2 = session->audio.Create(sessparams,&transparams);
		
		if( ret1 != 0)
			init_ret = ret1;
		else if( ret2 != 0)
			init_ret = ret2;
		else
			init_ret = 0;
		return session;
	}
	
	inline static void DestroySession(Session * session) noexcept{
		if(session == nullptr)
			return;
		session->video.BYEDestroy(jrtplib::RTPTime(10,0),nullptr,0);
		session->audio.BYEDestroy(jrtplib::RTPTime(10,0),nullptr,0);
		Close_Socket(session->video_socket);
		Close_Socket(session->audio_socket);
		delete session;
	}
	
	inline static int init_socket(const uint16_t& port) noexcept {
		int sock=socket(AF_INET,SOCK_DGRAM,0);
		struct sockaddr_in local;
		local.sin_family=AF_INET;
		local.sin_port=htons(port);
		local.sin_addr.s_addr=inet_addr("0.0.0.0");
		
		int opt = 1;
		constexpr char api[] = "init_socket";
		if(setsockopt(sock, SOL_SOCKET,SO_REUSEADDR, static_cast<void*>(&opt), sizeof(opt))){
		   rtpserver::Logger::Warning_app(MessageNum::Socket_set_opt_error,api);
		   close(sock);
		   return -1;
	   }
		if(bind(sock,(struct sockaddr*)&local,sizeof(local))<0){
			rtpserver::Logger::Warning_app(MessageNum::Socket_bind_failed,api);
			close(sock);
			return -1;
		}
		return sock;
	}
	
	inline static void Close_Socket(int socket) noexcept{
		if(socket > 0)
			close(socket);
	}
};

} // namespace rtpserver

