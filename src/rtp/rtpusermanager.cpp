#include "rtpusermanager.h"
#include "queue.h"

rtpserver::RTPUserManager * rtpserver::RTPUserManager::user_manager = nullptr;

rtpserver::Room::Room()
{
	rtp_queue = new RTPQueue(this);
	rtcp_queue = new RTCPQueue(this);
}

rtpserver::Room::~Room()
{
	delete rtp_queue;
	delete rtcp_queue;
}

void rtpserver::Room::push_rtp_packet(rtpserver::RTPPacket *packet) noexcept
{
	std::shared_ptr<RTPPacket> p(packet);
	rtp_queue->push_one(p);
}

void rtpserver::Room::push_rtcp_packet(rtpserver::RTCPPacket *packet)noexcept
{
	std::shared_ptr<RTCPPacket> p(packet);
	rtcp_queue->push_one(p);
}
