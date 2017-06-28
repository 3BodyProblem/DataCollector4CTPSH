#ifndef __CTP_BASIC_CACHE_H__
#define __CTP_BASIC_CACHE_H__


#pragma warning(disable:4786)
#include <map>
#include <string>
#include <fstream>
#include <stdexcept>
#include "DataDump.h"
#include "../Configuration.h"
#include "ThostFtdcTraderApi.h"
#include "../Infrastructure/Lock.h"
#include "../Infrastructure/Thread.h"
#include "../Infrastructure/DateTime.h"


typedef std::map<unsigned int,unsigned int>					T_MAP_RATE;			///< �Ŵ���ӳ���[��Ʒ����,��Ʒ�Ŵ���]
typedef std::map<std::string,CThostFtdcInstrumentField>		T_MAP_BASEDATA;		///< �������ݻ���


/**
 * @class			CTPQuoImage
 * @brief			���г��Ļ������ݹ�����
 * @detail			�����ڳ�ʼ���Ľ׶Σ��ӽ��׷�����ȡ�����Ⱦ�̬������Ϣ��
					��������������Դ�Ϊ�ж������Ƿ�ת�������ݡ�
 * @note			�Ѿ����������ļ��е�"�ڻ�/Ȩ"����ѡ����趨�������˲���Ҫ�ķ�����Ʒ����Ϣ
 * @author			barry
 */
class CTPQuoImage : public CThostFtdcTraderSpi
{
public:
	CTPQuoImage();

	/**
	 * @brief				���»�������
	 * @return				����ȡ������Ʒ�������
	 */
	int						FreshCache();

	/**
	 * @brief				�ͷ�api��Դ
	 * @return				>=0			�ɹ�
							<0			ʧ��
	 */
	int						FreeApi();

	/**
	 * @brief				ȡ���ܶ��ĵ���Ʒ
	 * @param[out]			pszCodeList	��Ʒ�б�
	 * @param[in]			nListSize	��Ʒ�б���
	 * @return				���ظ���
	 */
	int						GetSubscribeCodeList( char (&pszCodeList)[1024*5][20], unsigned int nListSize );

	/**
	 * @brief				��ȡ�����
	 */
	operator T_MAP_BASEDATA&();

	/**
	 * @brief				��ȡ�Ŵ���
	 */
	static int				GetRate( unsigned int nKind );

protected:///< ���з�������
	/**
	 * @brief				���͵�¼�����
	 */
    void					SendLoginRequest();

	/**
	 * @brief				�������ͻ�������
	 */
	void					BuildBasicData();

	/**
	 * @brief				������Ӧ�ȴ�
	 */
	void					BreakOutWaitingResponse();

	/**
	 * @brief				���ݴ����ж���Ʒ����
	 */
	int						JudgeKindFromSecurityID( char* pszCode, unsigned int nCodeLen = 9 ) const;

	/**
	 * @brief				����Ʒ������ȡ��ִ������
	 */
	int						ParseExerciseDateFromCode( char (&Code)[20] ) const;

protected:///< CThostFtdcTraderSpi�Ļص��ӿ�
	void					OnFrontConnected();
	void					OnFrontDisconnected( int nReason );
	void					OnHeartBeatWarning( int nTimeLapse );
	void					OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );
	void					OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );
	void					OnRspQryInstrument( CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

protected:
	bool					m_bIsResponded;				///< �������
	unsigned short			m_nTrdReqID;				///< ����api����ID
	CThostFtdcTraderApi*	m_pTraderApi;				///< ����ģ��ӿ�
	static T_MAP_RATE		m_mapRate;					///< ������ķŴ���
	T_MAP_BASEDATA			m_mapBasicData;				///< �г���Ʒ�������ݼ���
	CriticalObject			m_oLock;					///< �ٽ�������
};



#endif






