
#pragma once

#include <stdint.h>

// 这里的数值应随着参考帧数量的增加而增加！
#define	MAX_PICTURE_COUNT		2
#define	INTERNAL_BUFFER_SIZE	MAX_PICTURE_COUNT

#define RTP_VERSION 2

//Check RTP Seq
//const int MAX_DROPOUT = 3000;
//const int MAX_MISORDER = 100;
//const int MIN_SEQUENTIAL = 2;
//const int RTP_SEQ_MOD = 1<<16;

//RTP报文的最大序号
//const int MAX_RTP_SEQ = (1<<16) - 1 ;
//同一帧内最大RTP序号间隔
//const int MAX_RTP_RANGE = 64 ;
//帧的最大时间戳
//const unsigned int MAX_FRAME_TIMESTAMP = (unsigned int)(-1);//(1<<32) - 1 ;
//帧间最大时间戳间隔，15秒，90KHZ
//const unsigned int MAX_FRAME_TIMESTAMP_RANGE = 15 * 90000 ;

//RTP PayLoad Type
#if _MSC_VER < 1700
#pragma warning(disable: 4482)
enum RTPPayloadType : uint8_t {
#else
enum class RTPPayloadType : uint8_t {
#endif
	RTP_PT_DIVX = 125,	//Divx
	RTP_PT_H264 = 99,	//H.264编码的RTP Payload代码 Non-Interleaved模式
	RTP_PT_H263 = 96,	//H.263+编码的RTP Payload代码
	//RTP_PT_H261 = 31,	//H.261编码的RTP Payload代码
	RTP_PT_UNKNOWN = 255
};


const uint32_t MAX_FRAME_COUNT = 4;			// 单个channel帧队列的最大长度
const int MAX_FRAME_TIME_RANGE = 2 * 90000;	// 队列中帧间最大时间戳间隔，2秒，90KHZ

//
const int H264_NAL_FU = 28;
const int H264_NAL_STAP_A = 24;

//added by zyc
#define  MAX_RTP_IDLE_TIME 15       //RTP报文超时值(10秒),超过此时间仍未收到RTP报文,则认为该源已退出

#define  MAX_RTCP_IDLE_TIME 15      /*RTCP报文超时值（15秒）,超过此时间仍未收到报文，则认为该源已退出 */

#define RTCP_PT_SR	    200	/* sender report */
#define RTCP_PT_RR	    201	/* receiver report */
#define RTCP_PT_SDES	202	/* source description */
#define RTCP_PT_BYE	    203	/* end of participation */
#define RTCP_PT_APP	    204	/* application specific functions */

#define RTCP_SDES_NONE	0	/* end of SDES */
#define RTCP_SDES_CNAME	1	/* official name (mandatory) */
#define RTCP_SDES_NAME	2	/* personal name (optional) */
#define RTCP_SDES_EMAIL	3	/* e-mail addr (optional) */
#define RTCP_SDES_PHONE	4	/* telephone # (optional) */
#define RTCP_SDES_LOC	5	/* geographical location */
#define RTCP_SDES_TOOL	6	/* name/(vers) of app */
#define RTCP_SDES_NOTE	7	/* transient messages */
#define RTCP_SDES_PRIV	8	/* private SDES extensions */

#define	RTCP_SDES_MAX	8

//VS2 中MBusClient2的 地址参数
#define m_sMyMedia     "Video"
#define m_sMyModule    "Admire"
#define m_sMyApp       "VS2"

//VS2 中MBus的消息
#define VS2_VIDEODRAW_START_REQ		(WM_APP + 1024)
#define VS2_VIDEODRAW_STOP_REQ		(WM_APP + 1025)

#define VS2_NETINFO_REQ		        (WM_APP + 1027)
#define VS2_VIDEOSTART_REQ			(WM_APP + 1028)
#define VS2_VIDEOSTOP_REQ           (WM_APP + 1029)
#define VS2_SETCAP_REQ			    (WM_APP + 1030)
#define VS2_QUERYSENDING_REQ		(WM_APP + 1031)
#define VS2_VIDEOLIST_REQ           (WM_APP + 1032)
#define VS2_VIDEOREFRESH_REQ        (WM_APP + 1033)
#define VS2_VIDEOPARA_REQ           (WM_APP + 1034)
#define VS2_SETVIDEOPARA_REQ        (WM_APP + 1035)
#define VS2_KEYFRAME_SEND_REQ	    (WM_APP + 1036)

//added by wfc
#define VS2_SCSTART_REQ			    (WM_APP + 1037)
#define  VS2_SCSTOP_REQ			    (WM_APP + 1038)

//added by wmy
#define VS2_ADDDEVICE_REQ		    (WM_APP + 1039)
#define VS2_REMOVEDEVICE_REQ	    (WM_APP + 1040)

//added by chengrui
#define VS2_SET_RECVBUF_REQ			WM_APP + 1047
#define VS2_SET_SENDBUF_REQ			WM_APP + 1048
#define VS2_RESEND_REQ				WM_APP + 1049
#define VS2_SEND_RESEND_CMD			WM_APP + 1050
#define VS2_LOSTRATE_REQ			WM_APP + 1051

#define VS2_SETSEND_REQ				WM_APP + 1052

#ifdef VS2_TILE_OUTPUT
//added by louyihua
#define VS2_VIDEOSTREAM_LIST	    (WM_APP + 1042)
#define VS2_START_OUTPUT		    (WM_APP + 1043)
#define VS2_STOP_OUTPUT			    (WM_APP + 1044)
#endif

#if _MSC_VER < 1700
#pragma warning(disable: 4482)
enum CaptureType {
#else
enum class CaptureType {
#endif
	WDM,
	SCREEN,
	MEDIAFILE,
	UNKNOWN = -1
};

#if _MSC_VER < 1700
#pragma warning(disable: 4482)
enum StereoRenderMode {
#else
enum class StereoRenderMode {
#endif
	MONO, RED_CYAN_LR, GREEN_MAGENTA_LR, YELLOW_BLUE_LR, STEREO_MODE_MAX
};

#if _MSC_VER < 1700
#pragma warning(disable: 4482)
enum StereoFrameMode : uint8_t {
#else
enum class StereoFrameMode : uint8_t {
#endif
	STEREO_NONE, STEREO_HALF_LR, STEREO_FLAG_MAX
};
