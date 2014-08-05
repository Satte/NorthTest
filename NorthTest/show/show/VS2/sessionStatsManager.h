#pragma once

#include <list>
#include <semaphore.h>
#include "Types.h"
#include "rtcp.h"
#include "rtcpSender.h"
#include "RWLock.h"

class SessionStatsManager 
{
public:
	SessionStatsManager();
	~SessionStatsManager();
    
	//Read读保护，防止对revList的读时，有其他读线程的干预
	void getReadLockRevList() { m_lockRevList.acquireReadLock(); }
	//释放读revList保护锁
	void releaseReadLockRevList() { m_lockRevList.releaseReadLock(); }
	//Write锁，防止对revList写时，有其他写线程和读线程的干预
    void getWriteLockRevList() { m_lockRevList.acquireWriteLock(); }  
	//释放Write锁
    void releaseWriteLockRevList() { m_lockRevList.releaseWriteLock(); } 

	//Read读保护，防止对srcList的读时，有其他读线程的干预
	void getReadLockSrcList() { m_lockSrcList.acquireReadLock(); }
	//释放读srcList保护锁
	void releaseReadLockSrcList() { m_lockSrcList.releaseReadLock(); }
	//Write锁，防止对srcList写时，有其他写线程和读线程的干预
	void getWriteLockSrcList() { m_lockSrcList.acquireWriteLock(); } 
	//释放Write锁
	void releaseWriteLockSrcList() { m_lockSrcList.releaseWriteLock(); }


	//设置rtcp发送参数，name，cname等
	void setRtcpInfo(const std::wstring& note);
	void changeRRRtcpIP();//更改IP地址

	//rtcp bye相关
	void setDeleteIMEvent(sem_t *deleteIM) { m_deleteIM = deleteIM; }
	void fastDelete() { if (m_deleteIM) sem_post(m_deleteIM); }


	//根据SSRC在revList中搜索对应的视频源描述符结点
	bool findRev(unsigned int ssrc);
	//根据SSRC在revList中删除对应的视频源描述符结点
	void deleteRev(unsigned int ssrc) ;
	void setDeleteRev(unsigned int ssrc);
	//新加入一个视频源描述符结点在revList中
	revDescriptor* addRev(unsigned int ssrc) ;
	//删除整个revList链表
	void deleteTotalRev();
	//得到revList的链表头指针
	std::list<revDescriptor*>& getRevList() { return m_revList; }


	//返回本地有几路发送视频
	int numOfSrc() const { return m_Src; }
	void incrSrc(){ m_Src++; }
	void decrSrc(){ m_Src--; if (m_Src < 0) m_Src = 0; }

	//根据SSRC在srcList中搜索对应的视频源描述符结点
    bool findSrc(unsigned int ssrc);
	//根据SSRC在srcList中删除对应的视频源描述符结点
	void deleteSrc(unsigned int ssrc) ;
	//新加入一个视频源描述符结点在srcList中
	srcDescriptor* addSrc();
	//删除整个srcList链表
	void deleteTotalSrc();
	//改变一个源的SSRC
	void changeSSRC(unsigned int oldSSRC);

	const std::wstring& getUserName() const { return m_sUserName; }
	void setUserName(const std::wstring& value) { m_sUserName = value; }

	const std::wstring& getCName() const { return m_sCName; }
	void setCName(const std::wstring& value) { m_sCName = value; }

	const std::wstring& getName() const { return m_sName; }
	void setName(const std::wstring& value) { m_sName = value; }

	const std::wstring& getEmail() const { return m_sEmail; }
	void setEmail(const std::wstring& value) { m_sEmail = value; }

	const std::wstring& getPhone() const { return m_sPhone; }
	void setPhone(const std::wstring& value) { m_sPhone = value; }

	const std::wstring& getLocation() const { return m_sLocation; }
	void setLocation(const std::wstring& value) { m_sLocation = value; }

	const std::wstring& getTool() const { return m_sTool; }

	const std::wstring& getPrivate() const { return m_sPrivate; }

	uint32_t getNetAddr() const { return m_netAddr; }			//获取UI通过启动命令行指定的本地IP
	void setNetAddr(uint32_t value) { m_netAddr = value; }

	uint16_t getRtpPort() const { return m_netPort; }			//获取UI通过启动命令行指定的本地RTP端口号
	void setRtpPort(uint16_t value) { m_netPort = value; }

	uint16_t getRtcpPort() const { return m_netPort + 1; }		//获取UI通过启动命令行指定的本地RTCP端口号

	void setLocalAddr(uint32_t value) { m_localAddr = value; }

	uint8_t getTTL() const { return m_iTTL; }
	void setTTL(uint8_t value) { m_iTTL = value; }

	bool IsUseNat() const { return m_useNat; }			//是否使用NAT模式
	void setUseNat(bool value) { m_useNat = value; }

private:
	std::list<revDescriptor*>  m_revList;    //接受到的视频源属性描述链表
	std::list<srcDescriptor*>  m_srcList;    //本地的视频源描述符链表
	RTCPSender	m_rtcpSend;		//Rtcp报文发送器
	RWLock		m_lockRevList;	//对“接受到的视频源属性描述链表”加锁，保持删除和读取的互斥
	RWLock		m_lockSrcList;	//对“本地视频源属性描述链表”加锁，保持删除和读取的互斥
	sem_t       *m_deleteIM;		//提示RecvManager立即执行检查接收源存活情况的例程（本地收到了接受源的rtcp bye报文时）
	srcDescriptor* m_rr_SrcDpt;	//跟当本地只接受不发送时，发送rr rtcp相关的各个数据
	int			m_Src;			//本地有几路视频发送源

	//user information
	std::wstring  m_sUserName;       //UI登陆的用户名
	std::wstring  m_sName;            //用户姓名
	std::wstring  m_sCName;           //用户的别名
	std::wstring  m_sEmail;            //用户电子邮件地址
	std::wstring  m_sPhone;           //用户的地址
	std::wstring  m_sLocation;            //用户地址
	std::wstring  m_sTool;
	std::wstring  m_sPrivate;

	//programme information
	uint32_t m_localAddr ;      //本地IP地址
	uint32_t m_netAddr;         //目的IP地址
	uint16_t m_netPort;         //目的端口
	uint8_t  m_iTTL;
	bool	 m_useNat;        //是否使用nat

	std::wstring GetToolName();
};

extern SessionStatsManager g_SessionStateMgr;
