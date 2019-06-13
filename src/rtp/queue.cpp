
#include "queue.h"
#include "rtpusermanager.h"
#include "rtpsession.h"

namespace rtpserver {

RTPQueue::RTPQueue(rtpserver::Room *r):
	room(r)
{
	start_thread();
}

RTPQueue::~RTPQueue() 
{
	exit_thread();
}

void RTPQueue::on_thread_run() 
{
	if(wait_for_resource_push(1000)){
		deal_with_packet(get_next());
	}
}

void RTPQueue::deal_with_packet(AbstractQueue::pointer pkt)
{
	room->send_packet(pkt);
}

////////////////////////////////////////////////////////////////////////////////

RTCPQueue::RTCPQueue(Room *r):
	room(r)
{
	start_thread();
}

RTCPQueue::~RTCPQueue() 
{
	exit_thread();
}

void RTCPQueue::on_thread_run() 
{
	if(wait_for_resource_push(1000)){
		deal_with_packet(get_next());
	}
}

void RTCPQueue::deal_with_packet(AbstractQueue::pointer pkt)
{
	if(pkt->is_join){
		room->user_join(pkt->name,pkt->is_sender,pkt->SSRC,pkt->ip,pkt->rtp_port,pkt->rtcp_port);
	}
	else {
		room->user_exit(pkt->name,pkt->SSRC);
	}
}

}

