
#pragma once

namespace rtpserver {

constexpr char MessageString[][128] = {
	"rtp ( {} session ) listening port({}) success",
	"rtp ( {} session ) listening port({}) failed",
	"rtp ( {} session ) create destination(ip:{}.{}.{}.{},port:{}) success",
	"rtp ( {} session ) create destination(ip:{}.{}.{}.{},port:{}) failed",
	"rtp ( {} session ) set payload type failed",
	"rtp ( {} session ) set mark failed",
	"rtp ( {} session ) set timestamp increment failed",
	"rtp send packet failed",
	"rtp send packet success ( size:{} )",
	"function is abnormal because there is no {} session set up",
	"rtp destroy session(reason:{})",
	"(rtcp)adding users has an unexpected error,new(name[{}],ssrc[{}]),old(name[{}],ssrc1[{}],ssrc2[{}])",
	"(rtcp)insert user successful,name[{}],address1[{}:{},{}],address2[{}:{},{}],ssrc1[{}],ssrc2[{}]",
	"(rtcp)unexpected data encountered when removed,user name is [{}]",
	"(rtcp)remove user successful,name[{}],address1[{}:{},{}],address2[{}:{},{}],ssrc1[{}],ssrc2[{}]",
	"(rtcp)change mode successful,name[{}],address1[{}:{},{}],address2[{}:{},{}],ssrc1[{}],ssrc2[{}]",
	"(rtcp)set user,name[{}],address1[{}:{},{}],address2[{}:{},{}],ssrc1[{}],ssrc2[{}]",
	"time setting must be greater than zero",
	"Socket setup failed",
	"Socket bind port failed"
};

enum struct MessageNum {
	Rtp_listening_port_base_success = 0,
	Rtp_listening_port_base_failed,
	Rtp_create_destination_success,
	Rtp_create_destination_failed,
	Rtp_set_payload_type_failed,
	Rtp_set_mark_failed,
	Rtp_set_timestamp_increment_failed,
	Rtp_send_packet_failed,
	Rtp_send_packet_success,
	Rtp_not_set_session,
	Rtp_destroy_session,
	Rtcp_insert_user_failed,
	Rtcp_insert_user_successful,
	Rtcp_remove_abnormal,
	Rtcp_remove_user_successful,
	Rtcp_mode_change,
	Rtcp_set_user_information,
	Timer_time_less_than_zero,
	Socket_set_opt_error,
	Socket_bind_failed
};

} // namespace rtpserver
