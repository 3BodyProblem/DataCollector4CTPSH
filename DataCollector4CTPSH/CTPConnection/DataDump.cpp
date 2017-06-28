#include "DataDump.h"


std::string	JoinPath( std::string sPath, std::string sFileName )
{
	unsigned int		nSepPos = sPath.length() - 1;

	if( sPath[nSepPos] == '/' || sPath[nSepPos] == '\\' ) {
		return sPath + sFileName;
	} else {
		return sPath + "/" + sFileName;
	}
}

std::string	GenFilePathByWeek( std::string sFolderPath, std::string sFileName, unsigned int nMkDate )
{
	char				pszTmp[32] = { 0 };
	DateTime			oDate( nMkDate/10000, nMkDate%10000/100, nMkDate%100 );
	std::string&		sNewPath = JoinPath( sFolderPath, sFileName );

	::itoa( oDate.GetDayOfWeek(), pszTmp, 10 );
	sNewPath += ".";sNewPath += pszTmp;
	return sNewPath;
}


namespace QuotationSync
{


bool StaticSaver::Init( const char* pszFilePath, unsigned int nTradingDay )
{
	return MemoDumper<CThostFtdcInstrumentField>::Open( false, pszFilePath, nTradingDay );
}

void StaticSaver::Release()
{
	MemoDumper<CThostFtdcInstrumentField>::Close();
}


bool StaticLoader::Init( const char* pszFilePath )
{
	return MemoDumper<CThostFtdcInstrumentField>::Open( true, pszFilePath, 0 );
}

void StaticLoader::Release()
{
	MemoDumper<CThostFtdcInstrumentField>::Close();
}


bool SnapSaver::Init( const char* pszFilePath, unsigned int nTradingDay )
{
	m_bAppendModel = false;		///< 不开启追加模式

	return MemoDumper<CThostFtdcDepthMarketDataField>::Open( false, pszFilePath, nTradingDay );
}


void SnapSaver::Release()
{
	m_bAppendModel = false;

	MemoDumper<CThostFtdcDepthMarketDataField>::Close();
}


bool SnapLoader::Init( const char* pszFilePath )
{
	return MemoDumper<CThostFtdcDepthMarketDataField>::Open( true, pszFilePath, 0 );
}


void SnapLoader::Release()
{
	MemoDumper<CThostFtdcDepthMarketDataField>::Close();
}


CTPSyncSaver::CTPSyncSaver()
{
}

CTPSyncSaver& CTPSyncSaver::GetHandle()
{
	static	CTPSyncSaver	obj;

	return obj;
}

bool CTPSyncSaver::Init( const char* pszFilePath, unsigned int nTradingDay, bool bIsStaticData )
{
	Release( bIsStaticData );

	if( false == bIsStaticData )
	{
		return m_oSnapSaver.Init( pszFilePath, nTradingDay );
	}
	else
	{
		return m_oStaticSaver.Init( pszFilePath, nTradingDay );
	}
}

void CTPSyncSaver::Release( bool bIsStaticData )
{
	if( false == bIsStaticData )
	{
		m_oSnapSaver.Release();
	}
	else
	{
		m_oStaticSaver.Release();
	}
}

int CTPSyncSaver::SaveStaticData( const CThostFtdcInstrumentField& refData )
{
	return m_oStaticSaver.Write( refData);
}

int CTPSyncSaver::SaveSnapData( const CThostFtdcDepthMarketDataField& refData )
{
	return m_oSnapSaver.Write( refData);
}


CTPSyncLoader::CTPSyncLoader()
{
}

CTPSyncLoader& CTPSyncLoader::GetHandle()
{
	static	CTPSyncLoader	obj;

	return obj;
}

bool CTPSyncLoader::Init( const char* pszFilePath, bool bIsStaticData )
{
	Release( bIsStaticData );

	if( false == bIsStaticData )
	{
		return m_oSnapLoader.Init( pszFilePath );
	}
	else
	{
		return m_oStaticLoader.Init( pszFilePath );
	}
}

void CTPSyncLoader::Release( bool bIsStaticData )
{
	if( false == bIsStaticData )
	{
		m_oSnapLoader.Release();
	}
	else
	{
		m_oStaticLoader.Release();
	}
}

int CTPSyncLoader::GetStaticData( CThostFtdcInstrumentField& refData )
{
	return m_oStaticLoader.Read( refData);
}

int CTPSyncLoader::GetSnapData( CThostFtdcDepthMarketDataField& refData )
{
	return m_oSnapLoader.Read( refData );
}


}







