#include <math.h>
#include <string>
#include <algorithm>
#include "CTPQuoImage.h"
#include "../DataCollector4CTPSH.h"
#pragma comment(lib, "./CTPConnection/thosttraderapi.lib")


T_MAP_RATE		CTPQuoImage::m_mapRate;
T_MAP_KIND		CTPQuoImage::m_mapKind;


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
	QuotationRecover		oDataRecover;

	if( 0 != oDataRecover.OpenFile( sFilePath.c_str() ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::LoadDataFile() : failed 2 open static file : %s", sFilePath.c_str() );
		return -1;
	}

	if( true == bEchoOnly )
	{
		::printf( "��Լ����,����������,��Լ����,��Լ�ڽ������ڴ���,��Ʒ����,��Ʒ����,�������,������,�м۵�����µ���,�м۵���С�µ���,�޼۵�����µ���,�޼۵���С�µ���,��Լ��������,��С�䶯��λ,\
������,������,������,��ʼ������,����������,��Լ��������״̬,��ǰ�Ƿ���,�ֲ�����,�ֲ���������,��ͷ��֤����,��ͷ��֤����,�Ƿ�ʹ�ô��߱�֤���㷨,������Ʒ����,ִ�м�,��Ȩ����,\
��Լ������Ʒ����,�������\n" );
	}

	while( true )
	{
		int							nLen = 0;
		CThostFtdcInstrumentField	oData = { 0 };

		if( (nLen=oDataRecover.Read( (char*)&oData, sizeof(CThostFtdcInstrumentField) )) <= 0  )
		{
			break;
		}

		if( true == bEchoOnly )
		{
			::printf( "%s,%s,%s,%s,%s,%c,%d,%d,%d,%d,%d,%d,%d,%f,%s,%s,%s,%s,%s,%c,%d,%c,%c,%f,%f,%c,%s,%f,%c,%f,%c\n", oData.InstrumentID, oData.ExchangeID, oData.InstrumentName, oData.ExchangeInstID, oData.ProductID
					, oData.ProductClass, oData.DeliveryYear, oData.DeliveryMonth, oData.MaxMarketOrderVolume, oData.MinMarketOrderVolume, oData.MaxLimitOrderVolume, oData.MinLimitOrderVolume, oData.VolumeMultiple
					, oData.PriceTick, oData.CreateDate, oData.OpenDate, oData.ExpireDate, oData.StartDelivDate, oData.EndDelivDate, oData.InstLifePhase, oData.IsTrading, oData.PositionType, oData.PositionDateType
					, oData.LongMarginRatio, oData.ShortMarginRatio, oData.MaxMarginSideAlgorithm, oData.UnderlyingInstrID, oData.StrikePrice, oData.OptionsType, oData.UnderlyingMultiple, oData.CombinationType );
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
	m_mapRate.clear();													///< ��շŴ���ӳ���
	m_mapKind.clear();

	if( false == Configuration::GetConfig().IsBroadcastModel() )
	{
		FreeApi();///< ���������� && ����api���ƶ���
		if( NULL == (m_pTraderApi=CThostFtdcTraderApi::CreateFtdcTraderApi()) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : error occur while creating CTP trade control api" );
			return -1;
		}

		char		pszTmpFile[1024] = { 0 };							///< ׼������ľ�̬��������
		::sprintf( pszTmpFile, "Trade_%u_%u.dmp", DateTime::Now().DateToLong(), DateTime::Now().TimeToLong() );
		m_oDataRecorder.OpenFile( JoinPath( Configuration::GetConfig().GetDumpFolder(), pszTmpFile ).c_str(), true );					///< ׼���������ļ��ľ��

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

		FreeApi();														///< �ͷ�api+�����ļ��������������
	}
	else
	{
		if( LoadDataFile( Configuration::GetConfig().GetTradeFilePath().c_str(), false ) < 0 )
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

	///< ���÷�����Ϣ
	for( T_MAP_BASEDATA::iterator it = m_mapBasicData.begin(); it != m_mapBasicData.end(); it++ )
	{
		CThostFtdcInstrumentField&			refData = it->second;
		std::string							sCodeKey( refData.UnderlyingInstrID );

		if( m_mapKind.find( sCodeKey ) == m_mapKind.end() )
		{
			tagSHFutureKindDetail_LF108&	refKind = m_mapKind[sCodeKey];

			::memset( &refKind, 0, sizeof(refKind) );
			::sprintf( refKind.Key, "%u", m_mapKind.size()-1 );
			::strcpy( refKind.KindName, refData.UnderlyingInstrID );
			::strcpy( refKind.UnderlyingCode, refData.UnderlyingInstrID );
			refKind.PriceRate = 2;
			refKind.LotSize = 1;
			refKind.LotFactor = 100;
			refKind.PriceTick = refData.PriceTick * ::pow( (double)10, (int)refKind.PriceRate );
			refKind.ContractMult = refData.VolumeMultiple;
			refKind.DerivativeType = (THOST_FTDC_CP_CallOptions==refData.OptionsType) ? 0 : 1;
			refKind.PeriodsCount = 4;
			refKind.MarketPeriods[0][0] = 21*60;			///< ��һ�Σ�ȡҹ�̵�ʱ�ε����Χ
			refKind.MarketPeriods[0][1] = 23*60+30;
			refKind.MarketPeriods[1][0] = 9*60;				///< �ڶ���
			refKind.MarketPeriods[1][1] = 10*60+15;
			refKind.MarketPeriods[2][0] = 10*60+30;			///< ������
			refKind.MarketPeriods[2][1] = 11*60+30;
			refKind.MarketPeriods[3][0] = 13*60+30;			///< ���Ķ�
			refKind.MarketPeriods[3][1] = 15*60;

			m_mapRate[::atoi(refKind.Key)] = refKind.PriceRate;
			QuoCollector::GetCollector()->OnImage( 108, (char*)&refKind, sizeof(tagSHFutureKindDetail_LF108), true );
		}
	}

	tagMkInfo.KindCount = m_mapKind.size();
	::strcpy( tagStatus.Key, "mkstatus" );
	tagStatus.MarketStatus = 0;
	tagStatus.MarketTime = DateTime::Now().TimeToLong();

	QuoCollector::GetCollector()->OnImage( 107, (char*)&tagMkInfo, sizeof(tagMkInfo), true );
	QuoCollector::GetCollector()->OnImage( 109, (char*)&tagStatus, sizeof(tagStatus), true );
}

int CTPQuoImage::FreeApi()
{
	m_nTrdReqID = 0;						///< ��������ID
	m_bIsResponded = false;					///< ������Ӧ��ɱ�ʶ
	m_oDataRecorder.CloseFile();			///< �ر������ļ����

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
			m_oDataRecorder.Record( (char*)pInstrument, sizeof(CThostFtdcInstrumentField) );
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

			T_MAP_KIND::iterator it = m_mapKind.find( refSnap.UnderlyingInstrID );
			if( it != m_mapKind.end() )
			{
				tagName.Kind = ::atoi( it->second.Key );
				tagName.DeliveryDate = ::atol(refSnap.StartDelivDate);						///< ������(YYYYMMDD)
				tagName.StartDate = ::atol(refSnap.OpenDate);								///< �׸�������(YYYYMMDD)
				tagName.EndDate = ::atol(refSnap.ExpireDate);								///< �������(YYYYMMDD), �� ������
				tagName.ExpireDate = tagName.EndDate;										///< ������(YYYYMMDD)
				::strncpy( tagName.Name, refSnap.InstrumentName, sizeof(tagName.Code) );	///< ��Ʒ����
				tagName.XqPrice = refSnap.StrikePrice*GetRate(tagName.Kind)+0.5;		///< ��Ȩ�۸�(��ȷ����) //[*�Ŵ���] 

				m_mapBasicData[std::string(pInstrument->InstrumentID)] = *pInstrument;
				QuoCollector::GetCollector()->OnImage( 110, (char*)&tagName, sizeof(tagName), bIsLast );
				QuoCollector::GetCollector()->OnImage( 111, (char*)&tagSnapLF, sizeof(tagSnapLF), bIsLast );
				QuoCollector::GetCollector()->OnImage( 112, (char*)&tagSnapHF, sizeof(tagSnapHF), bIsLast );
				QuoCollector::GetCollector()->OnImage( 113, (char*)&tagSnapBS, sizeof(tagSnapBS), bIsLast );
			}
			else
			{
				QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnRspQryInstrument() : ignore invalid kind index, code=%s, underlyingcode=%s", refSnap.InstrumentID, refSnap.UnderlyingInstrID );
			}
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








