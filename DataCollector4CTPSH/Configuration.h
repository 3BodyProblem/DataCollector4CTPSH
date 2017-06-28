#ifndef __DATA_COLLECTOR_H__
#define __DATA_COLLECTOR_H__


#pragma warning(disable: 4786)
#include <vector>
#include <string>
#include "Infrastructure/Lock.h"
#include "CTPConnection/ThostFtdcMdApi.h"
#include "CTPConnection/ThostFtdcTraderApi.h"


extern	HMODULE						g_oModule;						///< ��ǰdllģ�����


/**
 * @class						CTPLinkConfig
 * @brief						һ��CTP������Ϣ
 * @date						2017/5/15
 * @author						barry
 */
class CTPLinkConfig
{
public:
	/**
	 * @brief					ע��ǰ�ú����Ʒ�����
	 * @param[in]				pHQApi				CTP����ӿ�
	 * @param[in]				pTrdApi				CTP���׽ӿ�
	 * @return					true				�ɹ�
	 */
	bool						RegisterServer( CThostFtdcMdApi* pHQApi, CThostFtdcTraderApi* pTrdApi );

public:
    std::string					m_sParticipant;			///< �����߱��
    std::string					m_sUID;					///< �û�ID
    std::string					m_sPswd;				///< ��¼����
	std::vector<std::string>	m_vctFrontServer;		///< ǰ�÷�������ַ
	std::vector<std::string>	m_vctNameServer;		///< ���Ʒ�������ַ
};


/**
 * @class						Configuration
 * @brief						������Ϣ
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					��ȡ���ö���ĵ������ö���
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					����������
	 * @return					==0					�ɹ�
								!=					����
	 */
    int							Initialize();

public:
	/**
	 * @brief					ȡ�ÿ�������Ŀ¼(���ļ���)
	 */
	const std::string&			GetDumpFolder() const;

	/**
	 * @brief					��ȡ���г�������·���ñ�
	 */
	CTPLinkConfig&				GetHQConfList();

	/**
	 * @brief					��ȡ���г�������·���ñ�
	 */
	CTPLinkConfig&				GetTrdConfList();

	/**
	 * @brief					��ȡ���������
	 */
	const std::string&			GetExchangeID() const;

	/**
	 * @brief					��ȡ�г����
	 */
	unsigned int				GetMarketID() const;

private:
	unsigned int				m_nMarketID;			///< �г����
	std::string					m_sExchangeID;			///< ���������
	std::string					m_sDumpFileFolder;		///< ��������·��(��Ҫ���ļ���)
	CTPLinkConfig				m_oHQConfigList;		///< CTP������������������б�
	CTPLinkConfig				m_oTrdConfigList;		///< CTP���׷��������������б�
};


#endif







