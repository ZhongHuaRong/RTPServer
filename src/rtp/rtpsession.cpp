#include "rtpsession.h"
#include "jrtplib3/rtppacket.h"
#include "jrtplib3/rtpsourcedata.h"
#include "jrtplib3/rtpaddress.h"
#include "jrtplib3/rtcpcompoundpacket.h"
#include "jrtplib3/rtcpapppacket.h"
#include "jrtplib3/rtcpsrpacket.h"
#include "jrtplib3/rtcprrpacket.h"
#include "jrtplib3/rtcpbyepacket.h"
#include "jrtplib3/rtcpsdespacket.h"
#include "jrtplib3/rtpipv4address.h"
#include "../core/logger.h"
#include "rtpusermanager.h"

namespace rtpserver {

RTPSession::RTPSession()
{
	
}

RTPSession::~RTPSession()
{
	
}

void RTPSession::OnValidatedRTPPacket(jrtplib::RTPSourceData *srcdat, jrtplib::RTPPacket *rtppack, 
									  bool isonprobation, bool *ispackethandled)
{
	UNUSED(isonprobation)
	*ispackethandled = true;
	std::shared_ptr<jrtplib::RTPPacket> ptr(rtppack);
	RTPUserManager::GetObject()->deal_with_rtp_packet(new RTPPacket{srcdat,ptr});
}

void RTPSession::OnRTCPCompoundPacket(jrtplib::RTCPCompoundPacket *pack, const jrtplib::RTPTime &receivetime,
									  const jrtplib::RTPAddress *senderaddress) {
	UNUSED(receivetime)
	UNUSED(senderaddress)

	auto &ptr = pack;
	auto manager = RTPUserManager::GetObject();
	auto address = static_cast<const jrtplib::RTPIPv4Address*>(senderaddress);
	jrtplib::RTCPPacket * rtcp_packet = nullptr;
	ptr->GotoFirstPacket();
	uint16_t && rtp_port = address->GetPort();
//	uint16_t && rtcp_port = address->GetRTCPSendPort();
	uint16_t & rtcp_port = rtp_port;
	while( ( rtcp_packet = ptr->GetNextPacket() ) != nullptr ){
		
		switch(rtcp_packet->GetPacketType()){
		case jrtplib::RTCPPacket::PacketType::BYE:
		{
			auto packet = static_cast<jrtplib::RTCPBYEPacket*>(rtcp_packet);
			auto && count = packet->GetSSRCCount();
			for( int n = 0; n < count; ++n){
				auto ssrc = packet->GetSSRC(n);
				auto info = this->GetSourceInfo(ssrc);
				if(info == nullptr)
					continue;
				size_t len;
				void *data = info->SDES_GetName(&len);
				std::string name(static_cast<char*>(data),len);
				if(len == 0)
					continue;
				data = info->SDES_GetNote(&len);
				std::string note(static_cast<char*>(data),len - 1);
				manager->deal_with_rtcp_packet(new RTCPPacket{
												   false,
												   false,
												   packet->GetSSRC(n),
												   name,
												   note,
												   address->GetIP(),
												   rtp_port,
												   rtcp_port});
			}
			break;
		}
		case jrtplib::RTCPPacket::PacketType::SDES:
		{
			auto packet = static_cast<jrtplib::RTCPSDESPacket*>(rtcp_packet);
			if(!packet->GotoFirstChunk())
				break;
			do{
				deal_with_sdes(packet,address->GetIP(),rtp_port,rtcp_port);
			}while(packet->GotoNextChunk());
			break;
		}
		default:
			break;
		}
	}
}

void RTPSession::deal_with_sdes(jrtplib::RTCPSDESPacket* sdes,
								const uint32_t& ip,
								const uint16_t &rtp_port,
								const uint16_t &rtcp_port) noexcept
{
	auto &packet = sdes;
	
	if(!packet->GotoFirstItem())
		return;
	
	auto && ssrc = packet->GetChunkSSRC();
	std::string name,note;
	void * p;
	
	do{
		//先将指针值设置为指向空的指针
		//可以方便转为任意类型的指针类型
		p = packet->GetItemData();
		switch (packet->GetItemType()) {
		case jrtplib::RTCPSDESPacket::CNAME:
			//CNAME也不关心，因为不允许我设置
			break;
		case jrtplib::RTCPSDESPacket::NAME:
			name.append(static_cast<char*>(p),packet->GetItemLength());
			break;
		case jrtplib::RTCPSDESPacket::NOTE:
			note.append(static_cast<char*>(p),packet->GetItemLength());
			break;
		default:
			//其他字段不关心
			break;
		}
	}while(packet->GotoNextItem());
	
	if( note.size() <= 1) {
		//房间值都为0了，就是没加入房间，也没推流，没必要继续处理
		//只有推流标志位也不行，也就是没有加入房间
		return;
	}
	
	bool is_sender = ( note[note.length() - 1] == '1' );
	note.pop_back();
	
	RTPUserManager::GetObject()->deal_with_rtcp_packet(new RTCPPacket{
														   true,
														   is_sender,
														   ssrc,
														   name,
														   note,
														   ip,
														   rtp_port,
														   rtcp_port});
}

} // namespace rtpserver

