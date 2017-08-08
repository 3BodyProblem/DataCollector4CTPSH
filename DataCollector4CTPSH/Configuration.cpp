#include <string>
#include <algorithm>
#include "Configuration.h"
#include "DataCollector4CTPSH.h"
#include "Infrastructure/IniFile.h"


HMODULE						g_oModule;


bool CTPLinkConfig::RegisterServer( CThostFtdcMdApi* pHQApi, CThostFtdcTraderApi* pTrdApi )
{
	std::vector<std::string>::iterator it;
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPLinkConfig::RegisterServer() : config server, [HQ Api Addr: %x],[Trd Api Addr: %x]", pHQApi, pTrdApi );

	if( m_vctFrontServer.empty() && m_vctNameServer.empty() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPLinkConfig::RegisterServer() : invalid connection configuration, ip list = None" );
		return false;
	}

	if( NULL == pHQApi && NULL == pTrdApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPLinkConfig::RegisterServer() : invalid ctp api pointer..." );
		return false;
	}

	for( it = m_vctFrontServer.begin(); it != m_vctFrontServer.end(); it++ )///< 配置前置服务器
	{
		if( !it->empty() )
		{
			if( NULL != pHQApi )
			{
				pHQApi->RegisterFront( (char*)(*it).c_str() );
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPLinkConfig::RegisterServer() : [HQ] FrontServer IP:  %s", (*it).c_str() );
			}
			else if( NULL != pTrdApi )
			{
				pTrdApi->RegisterFront( (char*)(*it).c_str() );
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPLinkConfig::RegisterServer() : [Trd] FrontServer IP:  %s", (*it).c_str() );
			}
		}
	}

	for( it = m_vctNameServer.begin(); it != m_vctNameServer.end(); it++ )///< 配置域名服务器
	{
		if( !it->empty() )
		{
			if( NULL != pHQApi )
			{
				pHQApi->RegisterNameServer( (char*)(*it).c_str() );
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPLinkConfig::RegisterServer() : [HQ] NameServer IP:  %s", (*it).c_str() );
			}
			else if( NULL != pTrdApi )
			{
				pTrdApi->RegisterNameServer( (char*)(*it).c_str() );
				QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPLinkConfig::RegisterServer() : [Trd] NameServer IP:  %s", (*it).c_str() );
			}
		}
	}

	return true;
}

///< 这是一个本文件内用的字符串split实现
std::vector<std::string> StrSplit( std::string sSvrIPs )
{
	std::vector<std::string>		vctIP;
	std::string::size_type			nNewPos = 0;
	std::string::size_type			nLastPos = 0;

	while( nLastPos < (sSvrIPs.length() + 1) )
	{
		nNewPos = sSvrIPs.find_first_of( ',', nLastPos );
		if( nNewPos == std::string::npos )
		{
			nNewPos = sSvrIPs.length();
		}

		std::string		sIP( sSvrIPs.data()+nLastPos, nNewPos-nLastPos );
		vctIP.push_back( sIP );
		nLastPos = nNewPos + 1;
	}

	return vctIP;
}


///< 解析并加载服务器连接登录配置信息
int ParseSvrConfig( inifile::IniFile& refIniFile, std::string sNodeName, CTPLinkConfig& oCTPConfig )
{
	int		nErrCode = 0;

	oCTPConfig.m_sParticipant = refIniFile.getStringValue( sNodeName, std::string("Participant"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::ParseSvrConfig() : invalid participant." );
		return -1;
	}

	oCTPConfig.m_sUID = refIniFile.getStringValue( sNodeName, std::string("LoginUser"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::ParseSvrConfig() : invalid login UserName." );
		return -2;
	}

	oCTPConfig.m_sPswd = refIniFile.getStringValue( sNodeName, std::string("LoginPWD"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::ParseSvrConfig() : invalid login LoginPWD." );
		return -3;
	}

	std::string		sFrontServer = refIniFile.getStringValue( sNodeName, std::string("FrontServer"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::ParseSvrConfig() : invalid FrontServer." );
		return -3;
	}
	oCTPConfig.m_vctFrontServer = StrSplit( sFrontServer );

	std::string		sNameServer = refIniFile.getStringValue( sNodeName, std::string("NameServer"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::ParseSvrConfig() : invalid NameServer." );
		return -4;
	}
	oCTPConfig.m_vctNameServer  = StrSplit( sNameServer );

	return 0;
}


Configuration::Configuration()
{
}

Configuration& Configuration::GetConfig()
{
	static Configuration	obj;

	return obj;
}

int Configuration::Initialize()
{
	std::string			sPath;
	inifile::IniFile	oIniFile;
	int					nErrCode = 0;
    char				pszTmp[1024] = { 0 };

	m_nMarketID = 15;
	m_sExchangeID = "SHFE";
    ::GetModuleFileName( g_oModule, pszTmp, sizeof(pszTmp) );
    sPath = pszTmp;
    sPath = sPath.substr( 0, sPath.find(".dll") ) + ".ini";
	if( 0 != (nErrCode=oIniFile.load( sPath )) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "Configuration::Initialize() : configuration file isn\'t exist. [%s], errorcode=%d", sPath.c_str(), nErrCode );
		return -1;
	}

	///< 设置： 快照落盘目录(含文件名)
	m_sDumpFileFolder = oIniFile.getStringValue( std::string("SRV"), std::string("DumpFolder"), nErrCode );
	if( 0 != nErrCode )	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "Configuration::Initialize() : shutdown dump function." );
	}

	if( 0 != ParseSvrConfig( oIniFile, "HQSRV", m_oHQConfigList ) )
	{
		return -2;
	}

	if( 0 != ParseSvrConfig( oIniFile, "TRDSRV", m_oTrdConfigList ) )
	{
		return -3;
	}

	return 0;
}

unsigned int Configuration::GetMarketID() const
{
	return m_nMarketID;
}

const std::string& Configuration::GetExchangeID() const
{
	return m_sExchangeID;
}

const std::string& Configuration::GetDumpFolder() const
{
	return m_sDumpFileFolder;
}

CTPLinkConfig& Configuration::GetHQConfList()
{
	return m_oHQConfigList;
}

CTPLinkConfig& Configuration::GetTrdConfList()
{
	return m_oTrdConfigList;
}










