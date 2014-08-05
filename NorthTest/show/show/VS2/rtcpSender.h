#ifndef  RTCPSENDER_H_
#define  RTCPSENDER_H_

#include "Types.h"	  
#include "rtcp.h"
#include "UDPSocket.h"
#include <pthread.h>
#include <unistd.h>


class RTCPSender
{
	static const int RTCP_MIN_TIME = 5000;		// RTCP 最小发送间隔（ms）
	static const int RTCP_MAX_TIME = 10000;		// RTCP 最大发送间隔（ms）
public:
	RTCPSender(bool bSender);
	~RTCPSender();

	void setRtcpInfo(const SDES& sdes, uint32_t dwIP, uint16_t iPort, uint8_t iTTL, bool bNat);	//初始化Sender的一些RTCP信息
	void changeRtcpInfo(uint32_t dwIP, uint16_t iPort, uint8_t iTTL);							//初始化Sender的一些RTCP信息
    void startSendRTCP(srcDescriptor* src);//开始发送RTCP报文,bw = 200kpbs
	bool stopSendRTCP();                	  //停止发送RTCP报文

private:
	char m_sdes[RTCP_SDES_MAX][256];		//发送源的CName和Name等信息，UTF-8形式
	CUDPSocketSend m_socket;				//负责发送的UDP Socket
	//Rtcp_Info m_rtcp_info;					//该结点的各种信息
	srcDescriptor* m_srcDpt;				//指向该视频发送源描述符的结点指针
    pthread_t m_sendThreadHdr;					//发送线程句柄
	uint32_t m_dwIP;       //目的IP地址
	uint16_t m_iPort;        //目的端口
	uint8_t m_iTTL;         //报文存活时间
	bool m_bNat;        //是否使用NAT模式
	const bool m_wesend;					//该视频源是否在发送
	
	static void *threadRTCPSend(void * param) { reinterpret_cast<RTCPSender*>(param)->SendProc(); return 0; }  //发送RTCP报文的子线程

	//根据情况自动判断发送RR或者SR
	void SendProc();

	//生成各种类型的RTCP报文
	void ProduceRTCP(TPacket& packet);
	void ProduceSR(TPacket& packet);
	void ProduceRR(TPacket& packet, bool bHasReports);
	void ProduceBye(TPacket& packet);
	void ProduceSDES(TPacket& packet);
	void ProduceAPP(TPacket& packet);

	int ProduceOneSDES(uint8_t* ptr, uint8_t code);
	int ProduceReport(rtcp_rr* rr);
    int WideCharToMultiByte(std::wstring inbuf, int insize, char *outbuf, int maxoutsize);
};
#endif
