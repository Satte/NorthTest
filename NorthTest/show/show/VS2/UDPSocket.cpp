//#include <string.h>
#include "UDPSocket.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CUDPSocketBase::OpenNATSocket(uint32_t ipServer, uint16_t iPort)
{
    /*
	WSAPROTOCOL_INFO info;
	if (!GetProviders(&info))
	{
		::MessageBox(nullptr, _T("装载NAT模块失败"), _T("VS2"), MB_OK | MB_ICONSTOP);
		exit(-1);
	}
	m_sock = WSASocket(AF_INET, SOCK_DGRAM, 0, &info, 0, 0);

	sockaddr_in servaddr;
	ZeroMemory(&servaddr,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = ipServer;//需要htonl吗，ipDst是怎么来的,htonl(inet_addr(""))？
	servaddr.sin_port = htons(iPort); // or try to use htons(8848);
	setsockopt(m_sock, SOL_SOCKET, PVD_CONFIG, (const char*)&servaddr, sizeof(servaddr));
    */
}

void CUDPSocketBase::OpenNormalSocket()
{
	m_sock = socket(AF_INET, SOCK_DGRAM, 0);
	assert(m_sock != INVALID_SOCKET);

	int reuse = 1;
	assert(setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse, sizeof(reuse)) != SOCKET_ERROR);
}

//CUDPSocketSend发送类的各个成员函数的定义
void CUDPSocketSend::Open(uint32_t ipDst, uint16_t iPortDst, int iTTL, bool bNat)
{
	//发送用的socket为UDP socket，绑定到INADDR_ANY，0端口，TTL=127
	if (m_sock != INVALID_SOCKET)
		Close();
	if (!bNat)
	{
		OpenNormalSocket();

		assert(setsockopt(m_sock, IPPROTO_IP, IP_TTL, (char*)&iTTL, sizeof(iTTL)) != SOCKET_ERROR);
	}
	else
	{
		OpenNATSocket(ipDst, 8848);
	}

	//即使要发送给组播组，也不必加入，只需要记录该地址即可
	memset(&m_saDst, 0, sizeof(m_saDst));
	m_saDst.sin_family = AF_INET;
	m_saDst.sin_addr.s_addr = ipDst;
	m_saDst.sin_port = htons(iPortDst);
}
   
void CUDPSocketSend::Close(void)
{
	if (m_sock == INVALID_SOCKET)
		return;

	close(m_sock);
	m_sock = INVALID_SOCKET;
}

int CUDPSocketSend::Send(const TPacket& packet)
{
	return sendto(m_sock, (char*)packet.Buffer, packet.Length, 0, (sockaddr*)&m_saDst, sizeof(m_saDst));
}


//============================================================================================================


//UDPSocketRecv接收类各个成员函数的定义
void CUDPSocketRecv::ReceiveProc()
{
	TPacket* pkt = new TPacket;

    struct pollfd aPoolfd;
    aPoolfd.fd = m_sock;
    aPoolfd.events = POLLIN|POLLPRI;

    while(1){
        switch(poll(&aPoolfd, 1, TIME_WAITFOR_PACKET)){
            case -1:
                break;
            case 0:                   // TIME OUT
                if ( m_bEndRecv ){
                    delete pkt;
                    return ;
                }
            default:
                if( (aPoolfd.revents & POLLIN) == POLLIN || (aPoolfd.revents & POLLPRI) == POLLPRI){
                    sockaddr_in saFrom;
                    int iFromLen = sizeof(saFrom);
                    pkt->Length = recvfrom(m_sock, (char*)pkt->Buffer, TPacket::MaxPacketLength, 0, (sockaddr*)&saFrom, (socklen_t *)&iFromLen);

                    if (pkt->Length > 0)
                    {
                        EnterCriticalSection(&m_csList);
                        // 两种情况
                        if (m_Packets.size() < MAX_UDP_QUEUE)
                        {
                            // 队列未满，则插入报文到末尾
                            m_Packets.push_back(pkt);
                        //    ReleaseSemaphore(m_hSemaPacket, 1, nullptr);
                            sem_post(m_hSemaPacket);
                            pkt = new TPacket;
                        }
                        else
                        {
                            // 队列已满，则将最早的报文空间用于存放下一个报文
                            m_Packets.push_back(pkt);
                            pkt = m_Packets.front();
                            m_Packets.pop_front();
                        }
                        LeaveCriticalSection(&m_csList);
                    }
                }
        }
    }
    delete pkt;

}

void CUDPSocketRecv::Open(int iPortListen, uint32_t ipGroup, sem_t *hSemaPacket, bool bNat)
{
	if (m_sock != INVALID_SOCKET) Close();

	if (!bNat)
		OpenNormalSocket();
	else
		OpenNATSocket(ipGroup, 0);

	m_hSemaPacket = hSemaPacket;        //报文到达的信号量

	//接收用的socket为UDP socket，绑定到INADDR_ANY iPortListen端口。如果ipGroup为组播地址，则加入ipGroup组
	sockaddr_in saBind;
	memset(&saBind, 0, sizeof(saBind));
	saBind.sin_family = AF_INET;
	saBind.sin_addr.s_addr = htonl(INADDR_ANY);
	saBind.sin_port = htons(iPortListen);
	assert(bind(m_sock, (sockaddr*)&saBind, sizeof(saBind)) != SOCKET_ERROR);

	if (IN_CLASSD(htonl(ipGroup)))
	{
		struct ip_mreq imr;
		imr.imr_multiaddr.s_addr = ipGroup;
		imr.imr_interface.s_addr = saBind.sin_addr.s_addr;
		assert(setsockopt(m_sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char*)&imr, sizeof(imr)) != SOCKET_ERROR);

		/* Disable loopback */
		bool bFlags = false;
		assert(setsockopt(m_sock, IPPROTO_IP, IP_MULTICAST_LOOP, (char *)&bFlags, sizeof(bFlags)) != SOCKET_ERROR);
	}

	//扩大接收的内核缓冲区
	int nRecvBuf = 512 * 1024;//设置为512K
	setsockopt(m_sock, SOL_SOCKET, SO_RCVBUF, (const char*)&nRecvBuf, sizeof(int));

	//设置报文到达事件触发线程
//	assert(WSAEventSelect(m_sock, m_hEvtRecv, FD_READ) != SOCKET_ERROR);

//	m_hThreadOnPacket = (HANDLE)_beginthreadex(nullptr, 0, threadOnPacket, this, 0, nullptr);

    pthread_create(&m_hThreadOnPacket, NULL, threadOnPacket, this);
}

void CUDPSocketRecv::Close(void)
{
	if (m_sock == INVALID_SOCKET)
		return;

	//首先停止监听，再关闭线程
    m_bEndRecv = true;
    pthread_join(m_hThreadOnPacket, NULL);
	close(m_sock);
	m_sock = INVALID_SOCKET;
}

TPacket* CUDPSocketRecv::GetPacket()
{
	EnterCriticalSection(&m_csList);
	TPacket* pkt = m_Packets.front();
	m_Packets.pop_front();
	LeaveCriticalSection(&m_csList);
	return pkt;
}
