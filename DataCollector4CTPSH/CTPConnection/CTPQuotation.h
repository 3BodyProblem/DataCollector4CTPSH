#ifndef __CTP_SESSION_H__
#define __CTP_SESSION_H__


#pragma warning(disable:4786)
#include <set>
#include <string>
#include <stdexcept>
#include "ThostFtdcMdApi.h"
#include "../Configuration.h"
#include "../CTP_SH_QuoProtocal.h"
#include "../Infrastructure/Lock.h"


/**
 * @class			CTPWorkStatus
 * @brief			CTP����״̬����
 * @author			barry
 */
class CTPWorkStatus
{
public:
	/**
	 * @brief				Ӧ״ֵ̬ӳ���״̬�ַ���
	 */
	static	std::string&	CastStatusStr( enum E_SS_Status eStatus );

public:
	/**
	 * @brief			����
	 * @param			eMkID			�г����
	 */
	CTPWorkStatus();
	CTPWorkStatus( const CTPWorkStatus& refStatus );

	/**
	 * @brief			��ֵ����
						ÿ��ֵ�仯������¼��־
	 */
	CTPWorkStatus&		operator= ( enum E_SS_Status eWorkStatus );

	/**
	 * @brief			����ת����
	 */
	operator			enum E_SS_Status();

private:
	CriticalObject		m_oLock;				///< �ٽ�������
	enum E_SS_Status	m_eWorkStatus;			///< ���鹤��״̬
};


typedef	std::set<std::string>		T_SET_INSTRUMENTID;			///< ��Ʒ���뼯��


/**
 * @class			CTPQuotation
 * @brief			CTP�Ự�������
 * @detail			��װ�������Ʒ�ڻ���Ȩ���г��ĳ�ʼ����������Ƶȷ���ķ���
 * @author			barry
 */
class CTPQuotation : public CThostFtdcMdSpi
{
public:
	CTPQuotation();
	~CTPQuotation();

	/**
	 * @brief			�ͷ�ctp����ӿ�
	 */
	int					Destroy();

	/**
	 * @brief			��ʼ��ctp����ӿ�
	 * @return			>=0			�ɹ�
						<0			����
	 * @note			������������������У�ֻ������ʱ��ʵ�ĵ���һ��
	 */
	int					Activate() throw(std::runtime_error);

protected:
	/**
	 * @brief			�����µ�������¶�������
	 * @return			>=0			�ɹ�
	 */
	int					SubscribeQuotation();

public:///< ������������
	/**
	 * @brief			��ȡ�Ự״̬��Ϣ
	 */
	CTPWorkStatus&		GetWorkStatus();

	/**
	 * @brief			��ȡ��������
	 */
	unsigned int		GetCodeCount();

	/**
	 * @brief			���͵�¼�����
	 */
    void				SendLoginRequest();

	/** 
	 * @brief			ˢ���ݵ����ͻ���
	 * @param[in]		pQuotationData			�������ݽṹ
	 * @param[in]		bInitialize				��ʼ�����ݵı�ʶ
	 */
	void				FlushQuotation( CThostFtdcDepthMarketDataField* pQuotationData, bool bInitialize );

public:///< CThostFtdcMdSpi�Ļص��ӿ�
	void OnFrontConnected();
	void OnFrontDisconnected( int nReason );
	void OnHeartBeatWarning( int nTimeLapse );
	void OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );
	void OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pMarketData );
	void OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

private:
	CThostFtdcMdApi*	m_pCTPApi;				///< �������ṩ��cffex�ӿ�
	CTPWorkStatus		m_oWorkStatus;			///< ����״̬
	T_SET_INSTRUMENTID	m_setRecvCode;			///< �յ��Ĵ��뼯��
	unsigned int		m_nCodeCount;			///< �յ��Ŀ�����Ʒ����
	CriticalObject		m_oLock;				///< �ٽ�������
};




#endif






