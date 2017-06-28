#ifndef __DATA_COLLECTOR_H__
#define __DATA_COLLECTOR_H__


#pragma warning(disable: 4786)
#include <vector>
#include <string>
#include "Infrastructure/Lock.h"
#include "CTPConnection/ThostFtdcMdApi.h"
#include "CTPConnection/ThostFtdcTraderApi.h"


extern	HMODULE						g_oModule;						///< 当前dll模块变量


/**
 * @class						CTPLinkConfig
 * @brief						一组CTP连接信息
 * @date						2017/5/15
 * @author						barry
 */
class CTPLinkConfig
{
public:
	/**
	 * @brief					注册前置和名称服务器
	 * @param[in]				pHQApi				CTP行情接口
	 * @param[in]				pTrdApi				CTP交易接口
	 * @return					true				成功
	 */
	bool						RegisterServer( CThostFtdcMdApi* pHQApi, CThostFtdcTraderApi* pTrdApi );

public:
    std::string					m_sParticipant;			///< 参与者编号
    std::string					m_sUID;					///< 用户ID
    std::string					m_sPswd;				///< 登录密码
	std::vector<std::string>	m_vctFrontServer;		///< 前置服务器地址
	std::vector<std::string>	m_vctNameServer;		///< 名称服务器地址
};


/**
 * @class						Configuration
 * @brief						配置信息
 * @date						2017/5/15
 * @author						barry
 */
class Configuration
{
protected:
	Configuration();

public:
	/**
	 * @brief					获取配置对象的单键引用对象
	 */
	static Configuration&		GetConfig();

	/**
	 * @brief					加载配置项
	 * @return					==0					成功
								!=					出错
	 */
    int							Initialize();

public:
	/**
	 * @brief					取得快照落盘目录(含文件名)
	 */
	const std::string&			GetDumpFolder() const;

	/**
	 * @brief					获取各市场行情链路配置表
	 */
	CTPLinkConfig&				GetHQConfList();

	/**
	 * @brief					获取各市场交易链路配置表
	 */
	CTPLinkConfig&				GetTrdConfList();

	/**
	 * @brief					获取交易所编号
	 */
	const std::string&			GetExchangeID() const;

	/**
	 * @brief					获取市场编号
	 */
	unsigned int				GetMarketID() const;

private:
	unsigned int				m_nMarketID;			///< 市场编号
	std::string					m_sExchangeID;			///< 交易所编号
	std::string					m_sDumpFileFolder;		///< 快照落盘路径(需要有文件名)
	CTPLinkConfig				m_oHQConfigList;		///< CTP行情服务器连接配置列表
	CTPLinkConfig				m_oTrdConfigList;		///< CTP交易服务器连接配置列表
};


#endif







