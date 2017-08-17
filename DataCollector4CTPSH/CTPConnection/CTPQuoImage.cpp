#include <math.h>
#include <string>
#include <algorithm>
#include "CTPQuoImage.h"
#include "../DataCollector4CTPSH.h"
#pragma comment(lib, "./CTPConnection/thosttraderapi.lib")


T_MAP_RATE		CTPQuoImage::m_mapRate;


CTPQuoImage::CTPQuoImage()
 : m_pTraderApi( NULL ), m_nTrdReqID( 0 ), m_bIsResponded( false )
{
}

CTPQuoImage::operator T_MAP_BASEDATA&()
{
	CriticalLock	section( m_oLock );

	return m_mapBasicData;
}

int CTPQuoImage::GetRate( unsigned int nKind )
{
	if( m_mapRate.find( nKind ) != m_mapRate.end() )
	{
		return ::pow( (double)10, (int)m_mapRate[nKind] );
	}

	return -1;
}

int CTPQuoImage::GetSubscribeCodeList( char (&pszCodeList)[1024*5][20], unsigned int nListSize )
{
	unsigned int	nRet = 0;
	CriticalLock	section( m_oLock );

	for( T_MAP_BASEDATA::iterator it = m_mapBasicData.begin(); it != m_mapBasicData.end() && nRet < nListSize; it++ ) {
		::strncpy( pszCodeList[nRet++], it->second.InstrumentID, 20 );
	}

	return nRet;
}

int CTPQuoImage::LoadDataFile( std::string sFilePath, bool bEchoOnly )
{
	if( false == QuotationSync::CTPSyncLoader::GetHandle().Init( sFilePath.c_str(), true ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::LoadDataFile() : failed 2 open static file : %s", sFilePath.c_str() );
		return -1;
	}

	while( true )
	{
		CThostFtdcInstrumentField	oData = { 0 };

		if( QuotationSync::CTPSyncLoader::GetHandle().GetStaticData( oData ) <= 0 )
		{
			break;
		}

		if( true == bEchoOnly )
		{
			::printf( "%s,%s\n", oData.InstrumentID, oData.InstrumentName );
		}
		else
		{
			OnRspQryInstrument( &oData, NULL, 0, false );
		}
	}

	return 0;
}

int CTPQuoImage::FreshCache()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::FreshCache() : ............ [%s] Freshing basic data ..............."
										, (false==Configuration::GetConfig().IsBroadcastModel())? "NORMAL" : "BROADCAST" );

	char			pszTmpFile[128] = { 0 };							///< ׼������ľ�̬��������
	unsigned int	nNowTime = DateTime::Now().TimeToLong();
	if( nNowTime > 80000 && nNowTime < 110000 )
		::strcpy( pszTmpFile, "Trade_am.dmp" );
	else
		::strcpy( pszTmpFile, "Trade_pm.dmp" );
	std::string		sDumpFile = GenFilePathByWeek( Configuration::GetConfig().GetDumpFolder().c_str(), pszTmpFile, DateTime::Now().DateToLong() );

	m_mapRate.clear();													///< ��շŴ���ӳ���

	if( false == Configuration::GetConfig().IsBroadcastModel() )
	{
		FreeApi();///< ���������� && ����api���ƶ���
		if( NULL == (m_pTraderApi=CThostFtdcTraderApi::CreateFtdcTraderApi()) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : error occur while creating CTP trade control api" );
			return -1;
		}

		QuotationSync::CTPSyncSaver::GetHandle().Init( sDumpFile.c_str(), DateTime::Now().DateToLong(), true );

		m_pTraderApi->RegisterSpi( this );								///< ��thisע��Ϊ�¼������ʵ��
		if( false == Configuration::GetConfig().GetTrdConfList().RegisterServer( NULL, m_pTraderApi ) )///< ע��CTP������Ҫ����������
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : invalid front/name server address" );
			return -2;
		}

		m_pTraderApi->Init();											///< ʹ�ͻ��˿�ʼ�����鷢����������������
		for( int nLoop = 0; false == m_bIsResponded; nLoop++ )			///< �ȴ�������Ӧ����
		{
			SimpleThread::Sleep( 1000 );
			if( nLoop > 60 * 3 ) {
				QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : overtime (>=3 min)" );
				return -3;
			}
		}

		FreeApi();														///< �ͷ�api����������
	}
	else
	{
		if( LoadDataFile( sDumpFile, false ) < 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : failed 2 load image data" );
			return -4;
		}
	}

	BuildBasicData();
	CriticalLock	section( m_oLock );
	unsigned int	nSize = m_mapBasicData.size();
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::FreshCache() : ............. [OK] basic data freshed(%d) ...........", nSize );

	return nSize;
}

void CTPQuoImage::BuildBasicData()
{
	tagSHFutureMarketInfo_LF107		tagMkInfo = { 0 };
	tagSHFutureMarketStatus_HF109	tagStatus = { 0 };

	::strcpy( tagMkInfo.Key, "mkinfo" );
	tagMkInfo.WareCount = m_mapBasicData.size();
	tagMkInfo.MarketID = Configuration::GetConfig().GetMarketID();
	tagMkInfo.MarketDate = DateTime::Now().DateToLong();

	tagMkInfo.PeriodsCount = 4;					///< ����ʱ����Ϣ����
	tagMkInfo.MarketPeriods[0][0] = 21*60;		///< ��һ�Σ�ȡҹ�̵�ʱ�ε����Χ
	tagMkInfo.MarketPeriods[0][1] = 23*60+30;
	tagMkInfo.MarketPeriods[1][0] = 9*60;		///< �ڶ���
	tagMkInfo.MarketPeriods[1][1] = 10*60+15;
	tagMkInfo.MarketPeriods[2][0] = 10*60+30;	///< ������
	tagMkInfo.MarketPeriods[2][1] = 11*60+30;
	tagMkInfo.MarketPeriods[3][0] = 13*60+30;	///< ���Ķ�
	tagMkInfo.MarketPeriods[3][1] = 15*60;

	///< ���÷�����Ϣ
	tagMkInfo.KindCount = 4;
	{
		tagSHFutureKindDetail_LF108		tagKind = { 0 };

		::strcpy( tagKind.Key, "0" );
		::strncpy( tagKind.KindName, "ָ������", 8 );
		tagKind.PriceRate = 0;
		tagKind.LotFactor = 0;
		m_mapRate[m_mapRate.size()] = tagKind.PriceRate;
		QuoCollector::GetCollector()->OnImage( 108, (char*)&tagKind, sizeof(tagKind), true );
	}
	{
		tagSHFutureKindDetail_LF108		tagKind = { 0 };

		::strcpy( tagKind.Key, "1" );
		::strncpy( tagKind.KindName, "�Ϻ���ָ", 8 );
		tagKind.PriceRate = 2;
		tagKind.LotFactor = 100;
		m_mapRate[m_mapRate.size()] = tagKind.PriceRate;
		QuoCollector::GetCollector()->OnImage( 108, (char*)&tagKind, sizeof(tagKind), true );
	}
	{
		tagSHFutureKindDetail_LF108		tagKind = { 0 };

		::strcpy( tagKind.Key, "2" );
		::strncpy( tagKind.KindName, "�Ϻ���Ȩ", 8 );
		tagKind.PriceRate = 2;
		tagKind.LotFactor = 100;
		m_mapRate[m_mapRate.size()] = tagKind.PriceRate;
		QuoCollector::GetCollector()->OnImage( 108, (char*)&tagKind, sizeof(tagKind), true );
	}

	::strcpy( tagStatus.Key, "mkstatus" );
	tagStatus.MarketStatus = 0;
	tagStatus.MarketTime = DateTime::Now().TimeToLong();

	QuoCollector::GetCollector()->OnImage( 107, (char*)&tagMkInfo, sizeof(tagMkInfo), true );
	QuoCollector::GetCollector()->OnImage( 109, (char*)&tagStatus, sizeof(tagStatus), true );
}

int CTPQuoImage::FreeApi()
{
	m_nTrdReqID = 0;						///< ��������ID
	m_bIsResponded = false;

	QuotationSync::CTPSyncSaver::GetHandle().Release( true );

	if( m_pTraderApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::FreeApi() : free api ......" );
		m_pTraderApi->Release();
		m_pTraderApi = NULL;
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::FreeApi() : done! ......" );
	}

	SimpleThread::Sleep( 1000*2 );

	return 0;
}

void CTPQuoImage::BreakOutWaitingResponse()
{
	CriticalLock	section( m_oLock );

	m_nTrdReqID = 0;						///< ��������ID
	m_bIsResponded = true;					///< ֹͣ�ȴ����ݷ���
	m_mapBasicData.clear();					///< ������л�������
}

void CTPQuoImage::SendLoginRequest()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::SendLoginRequest() : sending trd login message" );

	CThostFtdcReqUserLoginField	reqUserLogin = { 0 };

	memcpy( reqUserLogin.BrokerID, Configuration::GetConfig().GetTrdConfList().m_sParticipant.c_str(), Configuration::GetConfig().GetTrdConfList().m_sParticipant.length() );
	memcpy( reqUserLogin.UserID, Configuration::GetConfig().GetTrdConfList().m_sUID.c_str(), Configuration::GetConfig().GetTrdConfList().m_sUID.length() );
	memcpy( reqUserLogin.Password, Configuration::GetConfig().GetTrdConfList().m_sPswd.c_str(), Configuration::GetConfig().GetTrdConfList().m_sPswd.length() );
	strcpy( reqUserLogin.UserProductInfo,"�Ϻ�Ǭ¡�߿Ƽ����޹�˾" );
	strcpy( reqUserLogin.TradingDay, m_pTraderApi->GetTradingDay() );

	if( 0 == m_pTraderApi->ReqUserLogin( &reqUserLogin, 0 ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::SendLoginRequest() : login message sended!" );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::SendLoginRequest() : failed 2 send login message" );
	}
}

void CTPQuoImage::OnHeartBeatWarning( int nTimeLapse )
{
	QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnHeartBeatWarning() : hb overtime, (%d)", nTimeLapse );
}

void CTPQuoImage::OnFrontConnected()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnFrontConnected() : connection established." );

    SendLoginRequest();
}

void CTPQuoImage::OnFrontDisconnected( int nReason )
{
	const char*		pszInfo=nReason == 0x1001 ? "�����ʧ��" :
							nReason == 0x1002 ? "����дʧ��" :
							nReason == 0x2001 ? "����������ʱ" :
							nReason == 0x2002 ? "��������ʧ��" :
							nReason == 0x2003 ? "�յ�������" :
							"δ֪ԭ��";

	BreakOutWaitingResponse();

	QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnFrontDisconnected() : connection disconnected, [reason:%d] [info:%s]", nReason, pszInfo );
}

void CTPQuoImage::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
    if( pRspInfo->ErrorID != 0 )
	{
		// �˵�ʧ�ܣ��ͻ�������д�����
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnRspUserLogin() : failed 2 login [Err=%d,ErrMsg=%s,RequestID=%d,Chain=%d]"
											, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

		SimpleThread::Sleep( 1000*6 );
        SendLoginRequest();
    }
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnRspUserLogin() : succeed 2 login [RequestID=%d,Chain=%d]", nRequestID, bIsLast );

		///�����ѯ��Լ
		CThostFtdcQryInstrumentField	tagQueryParam = { 0 };
		::strncpy( tagQueryParam.ExchangeID, Configuration::GetConfig().GetExchangeID().c_str(), Configuration::GetConfig().GetExchangeID().length() );

		int	nRet = m_pTraderApi->ReqQryInstrument( &tagQueryParam, ++m_nTrdReqID );
		if( nRet >= 0 )
		{
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnRspUserLogin() : [OK] Instrument list requested! errorcode=%d", nRet );
		}
		else
		{
			BreakOutWaitingResponse();
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnRspUserLogin() : [ERROR] Failed 2 request instrument list, errorcode=%d", nRet );
		}
	}
}

void CTPQuoImage::OnRspQryInstrument( CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	if( NULL != pRspInfo )
	{
		if( pRspInfo->ErrorID != 0 )		///< ��ѯʧ��
		{
			BreakOutWaitingResponse();

			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnRspQryInstrument() : failed 2 query instrument [Err=%d,ErrMsg=%s,RequestID=%d,Chain=%d]"
												, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

			return;
		}
	}

	if( NULL != pInstrument )
	{
		if( false == Configuration::GetConfig().IsBroadcastModel() )
		{
			QuotationSync::CTPSyncSaver::GetHandle().SaveStaticData( *pInstrument );
		}

		if( false == m_bIsResponded && pInstrument->ProductClass == THOST_FTDC_PC_Futures )
		{	///< �ж�Ϊ�Ƿ���Ҫ���˵���Ʒ
			CThostFtdcInstrumentField&		refSnap = *pInstrument;						///< ��������ӿڷ��ؽṹ
			CriticalLock					section( m_oLock );
			tagSHFutureReferenceData_LF110	tagName = { 0 };							///< ��Ʒ������Ϣ�ṹ
			tagSHFutureSnapData_HF112		tagSnapHF = { 0 };							///< �����������
			tagSHFutureSnapData_LF111		tagSnapLF = { 0 };							///< �����������
			tagSHFutureSnapBuySell_HF113	tagSnapBS = { 0 };							///< ��λ��Ϣ

			::strncpy( tagName.Code, refSnap.InstrumentID, sizeof(tagName.Code) );		///< ��Ʒ����
			::memcpy( tagSnapHF.Code, refSnap.InstrumentID, sizeof(tagSnapHF.Code) );	///< ��Ʒ����
			::memcpy( tagSnapLF.Code, refSnap.InstrumentID, sizeof(tagSnapLF.Code) );	///< ��Ʒ����
			::memcpy( tagSnapBS.Code, refSnap.InstrumentID, sizeof(tagSnapBS.Code) );	///< ��Ʒ����

			tagName.Kind = 2;															///< ��Ȩ�ķ���
			tagName.DerivativeType = 0;
			tagName.DeliveryDate = ::atol(refSnap.StartDelivDate);						///< ������(YYYYMMDD)
			tagName.StartDate = ::atol(refSnap.OpenDate);								///< �׸�������(YYYYMMDD)
			tagName.EndDate = ::atol(refSnap.ExpireDate);								///< �������(YYYYMMDD), �� ������
			tagName.ExpireDate = tagName.EndDate;										///< ������(YYYYMMDD)
			tagName.ContractMult = refSnap.VolumeMultiple;
			::strncpy( tagName.Name, refSnap.InstrumentName, sizeof(tagName.Code) );	///< ��Ʒ����
			if( 3 == tagName.Kind )
			{
				tagName.LotSize = 1;													///< �ֱ��ʣ���ȨΪ1��
				::memcpy( tagName.UnderlyingCode, refSnap.UnderlyingInstrID, sizeof(tagName.UnderlyingCode) );///< ��Ĵ���
				tagName.XqPrice = refSnap.StrikePrice*GetRate(tagName.Kind)+0.5;		///< ��Ȩ�۸�(��ȷ����) //[*�Ŵ���] 
			}
			else if( 2 == tagName.Kind )
			{
				tagName.LotSize = 1;													///< �ֱ���
			}

			tagName.PriceTick = refSnap.PriceTick*m_mapRate[tagName.Kind]+0.5;							///< ��Ȩ�۸�(��ȷ����) //[*�Ŵ���] 

			m_mapBasicData[std::string(pInstrument->InstrumentID)] = *pInstrument;
			QuoCollector::GetCollector()->OnImage( 110, (char*)&tagName, sizeof(tagName), bIsLast );
			QuoCollector::GetCollector()->OnImage( 111, (char*)&tagSnapLF, sizeof(tagSnapLF), bIsLast );
			QuoCollector::GetCollector()->OnImage( 112, (char*)&tagSnapHF, sizeof(tagSnapHF), bIsLast );
			QuoCollector::GetCollector()->OnImage( 113, (char*)&tagSnapBS, sizeof(tagSnapBS), bIsLast );
		}
	}

	if( true == bIsLast )
	{	///< ���һ��
		CriticalLock	section( m_oLock );
		m_bIsResponded = true;				///< ֹͣ�ȴ����ݷ���(���󷵻����[OK])
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnRspQryInstrument() : [DONE] basic data freshed! size=%d", m_mapBasicData.size() );
	}
}

void CTPQuoImage::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuoImage::OnRspError() : ErrorCode=[%d], ErrorMsg=[%s],RequestID=[%d], Chain=[%d]"
										, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

	BreakOutWaitingResponse();
}








