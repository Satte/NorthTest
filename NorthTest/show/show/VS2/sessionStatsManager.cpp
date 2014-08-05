#include "sessionStatsManager.h"
#include "mediaTimer.h"
#include <algorithm>
#include <stdlib.h>
#include <sys/time.h>

SessionStatsManager g_SessionStateMgr;

SessionStatsManager::SessionStatsManager()
	: m_rtcpSend(false), m_Src(0), m_deleteIM(nullptr)
	, m_sUserName(L"no username")       //UI登陆的用户名
	, m_sName(L"no name")               //用户姓名
	, m_sEmail(L"no mail")               //用户电子邮件地址
	, m_sPhone(L"no phone")             //用户的地址
	, m_sLocation(L"no address")            //用户地址
	, m_sCName(L"no cname")             //用户的别名
	, m_localAddr(INADDR_ANY)        //本地IP地址
	, m_netAddr(INADDR_ANY)               //目的IP地址
	, m_netPort(8000)              //目的端口
	, m_useNat(false)                   //是否使用nat，默认不使用
	, m_sTool(GetToolName())
{
	//随机数种子初始化，用于产生ssrc
/*
    struct timeval tv;
    gettimeofday(&tv, NULL);
	srand((unsigned int)tv.tv_sec);
*/
	//媒体时钟初始化
//    MediaTimer::LoadPerformanceFrequency();
    //本地发送的rr的ssrc
	unsigned int ssrc = ((arc4random() & 0xffff) << 16) | (MediaTimer::GetTickCount() & 0xffff);
	m_rr_SrcDpt = new srcDescriptor(ssrc);
}

SessionStatsManager::~SessionStatsManager()
{
	deleteTotalSrc();
	deleteTotalRev();

	//停止RTCP报文的发送,一定要先于m_rr_SrcDpt解析前调用
	m_rtcpSend.stopSendRTCP();
	delete m_rr_SrcDpt;
}



void SessionStatsManager::setRtcpInfo(const std::wstring& note)
{
	//RTCP Sender RTCP信息初始化（设置name，cname等）
	m_rtcpSend.setRtcpInfo(SDES(m_sCName, m_sName, m_sEmail, m_sPhone, m_sLocation, m_sTool, note, m_sPrivate), m_netAddr, m_netPort + 1, m_iTTL, m_useNat);

	//发送rtcp的rr报文
	m_rtcpSend.startSendRTCP(m_rr_SrcDpt);
}


//更改IP地址
void SessionStatsManager::changeRRRtcpIP()
{
	//关闭RTCP报文发送线程
	m_rtcpSend.stopSendRTCP();

	//RTCP Sender重新初始化
	m_rtcpSend.changeRtcpInfo(m_netAddr, m_netPort + 1, m_iTTL);

	//重新发送rtcp的RR报文
	m_rtcpSend.startSendRTCP(m_rr_SrcDpt);
}



//根据SSRC在revList中搜索对应的视频源描述符结点
bool SessionStatsManager::findRev(unsigned int ssrc)
{
	 getReadLockRevList();

     bool result = (std::find_if (m_revList.begin(), m_revList.end(), [ssrc](revDescriptor* rev)->bool { return rev->m_ssrc == ssrc; }) != m_revList.end());
	 
	 releaseReadLockRevList();

	 return result ;
}

//设置删除标记(在收到RTCP的bye报文后)
void SessionStatsManager::setDeleteRev(unsigned int ssrc)
{
	 getReadLockRevList();
     
	 auto dpt = std::find_if (m_revList.begin(), m_revList.end(), [ssrc](revDescriptor* rev)->bool { return rev->m_ssrc == ssrc; }) ;
	 if (dpt != m_revList.end())
		 (*dpt)->m_bDelete = true;
	     
	 releaseReadLockRevList();
}

//根据SSRC在revList中删除对应的视频源描述符结点
void SessionStatsManager::deleteRev(unsigned int ssrc)
{
     getWriteLockRevList();
  
	 //先找到对应的revList中的描述符结点指针
	 auto dpt = std::find_if (m_revList.begin(), m_revList.end(), [ssrc](revDescriptor* rev)->bool { return rev->m_ssrc == ssrc; }) ;

	 //这些node都是new出来的对象，必须delete掉
	 if (dpt != m_revList.end())
	 {
		 delete (*dpt);
		 m_revList.erase(dpt);
	 }
	 
	 releaseWriteLockRevList();
}
//新加入一个视频源描述符结点在revList中
revDescriptor * SessionStatsManager::addRev(unsigned int ssrc)
{
     getWriteLockRevList();
     //必须保证在m_revList中不存在同样ssrc的接收信道描述符
	 //revDescriptor * dpt = NULL ;
	 //auto iterDpt = find_if (m_revList.begin(), m_revList.end(), [ssrc](revDescriptor* rev)->bool { return rev->m_ssrc == ssrc; });
	 //if (iterDpt == m_revList.end())
	 //{
		 revDescriptor * dpt = new revDescriptor(ssrc);
		 //dpt->m_iLastCalTS = MediaTimer::GetTickCount();
		 m_revList.push_front(dpt);
	 //} else
	 //	 dpt = *iterDpt ;
	 
	 releaseWriteLockRevList();
	 
	 return dpt;
}

//删除整个revList链表
void SessionStatsManager::deleteTotalRev()
{
	 getWriteLockRevList();
 
	 while(m_revList.size()>0)
	 {
		 delete m_revList.front();
		 m_revList.pop_front();
	 }
	 
	 releaseWriteLockRevList();
}

//根据SSRC在srcList中搜索对应的视频源描述符结点
bool SessionStatsManager::findSrc(unsigned int ssrc)
{
    getReadLockSrcList();

	bool result = (std::find_if (m_srcList.begin(), m_srcList.end(), [ssrc](srcDescriptor* src)->bool { return src->m_ssrc == ssrc; }) != m_srcList.end());
	
	releaseReadLockSrcList();         
	return result ;
}

//根据SSRC在srcList中删除对应的视频源描述符结点
void SessionStatsManager::deleteSrc(unsigned int ssrc)
{
	 getWriteLockSrcList();

	 //先找到对应的revList中的描述符结点指针
	 auto dpt = std::find_if (m_srcList.begin(), m_srcList.end(), [ssrc](srcDescriptor* src)->bool { return src->m_ssrc == ssrc; }) ;

	 //这些node都是new出来的对象，必须delete掉
	 if (dpt != m_srcList.end())
	 {
		 delete (*dpt);
      	 m_srcList.erase(dpt);
	 }

	 releaseWriteLockSrcList();
}

//新加入一个视频源描述符结点在srcList中
srcDescriptor * SessionStatsManager::addSrc()
{
	 getWriteLockSrcList();

	 unsigned int ssrc = ((rand() & 0xffff) << 16) | (MediaTimer::GetTickCount() & 0xffff);
	 srcDescriptor * srcDpt = new srcDescriptor(ssrc);
	 m_srcList.push_front(srcDpt);

	 releaseWriteLockSrcList();
    
	 return srcDpt ;
}

//改变一个源的SSRC(小概率事件，还是用写锁了)
void SessionStatsManager::changeSSRC(unsigned int oldSSRC)
{
	 getWriteLockSrcList();

	 auto dpt = std::find_if (m_srcList.begin(), m_srcList.end(), [oldSSRC](srcDescriptor* src)->bool { return src->m_ssrc == oldSSRC; }) ;
	 if (dpt != m_srcList.end())
	 {
		 //重新生成一个SSRC，所有统计信息都置零
		 (*dpt)->m_ssrc = ((rand() & 0xffff) << 16) | (MediaTimer::GetTickCount() & 0xffff);
		 (*dpt)->reset();
	 }
	
	 releaseWriteLockSrcList();
}

//删除整个srcList链表
void SessionStatsManager::deleteTotalSrc()
{
	 getWriteLockSrcList();

	 while(!m_srcList.empty())
	 {
		 delete m_srcList.front();
		 m_srcList.pop_front();
	 }

	 releaseWriteLockSrcList();
}

//得到VideoSender的版本信息，存入sToolName中
std::wstring SessionStatsManager::GetToolName()
{
    return L"no";
    /*
	std::wstring strTitle;
	wchar_t sModuleFilename[1024];

	GetModuleFileName(GetModuleHandle(nullptr), sModuleFilename, 1024);

	//版本信息长度
	int iLenRes = GetFileVersionInfoSize(sModuleFilename, nullptr);

	BYTE* pBuf = new BYTE[iLenRes];
	GetFileVersionInfo(sModuleFilename, 0, iLenRes, pBuf);

	VS_FIXEDFILEINFO* p;
	unsigned iDummy;
	VerQueryValue(pBuf, L"\\", (LPVOID*)&p, &iDummy);

	//版本信息
	strTitle.Format(L"VS2 %d.%d.%d.%d",
		HIWORD(p->dwProductVersionMS),
		LOWORD(p->dwProductVersionMS),
		HIWORD(p->dwProductVersionLS),
		LOWORD(p->dwProductVersionLS));

	delete []pBuf;
	return strTitle;
    */
}
