
#pragma once
#include "../core/abstractqueue.h"
#include "rtpsession.h"
#include "rtpusermanager.h"

namespace rtpserver {

struct Room;

class RTPQueue:public AbstractQueue<RTPPacket>{
public:
	RTPQueue(Room *r);
	
	virtual ~RTPQueue() override;
	
protected:
	inline virtual bool get_thread_pause_condition() noexcept override{
		return false;
	} 
	
	void on_thread_run() override;
	
	void deal_with_packet(pointer);
private:
	Room *room;
};

class RTCPQueue:public AbstractQueue<RTCPPacket>{	
public:
	RTCPQueue(Room *r);
	
	virtual ~RTCPQueue() override;
protected:
	inline virtual bool get_thread_pause_condition() noexcept override{
		return false;
	} 
	
	void on_thread_run() override;
	
	void deal_with_packet(pointer);
private:
	Room *room;
};

} //namespace rtpserver
