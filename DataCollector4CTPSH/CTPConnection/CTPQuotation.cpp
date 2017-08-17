#include <algorithm>
#include "CTPQuotation.h"
#include "../DataCollector4CTPSH.h"
#pragma comment(lib, "./CTPConnection/thostmduserapi.lib")


CTPWorkStatus::CTPWorkStatus()
: m_eWorkStatus( ET_SS_UNACTIVE )
{
}

CTPWorkStatus::CTPWorkStatus( const CTPWorkStatus& refStatus )
{
	CriticalLock	section( m_oLock );

	m_eWorkStatus = refStatus.m_eWorkStatus;
}

CTPWorkStatus::operator enum E_SS_Status()
{
	CriticalLock	section( m_oLock );

	return m_eWorkStatus;
}

std::string& CTPWorkStatus::CastStatusStr( enum E_SS_Status eStatus )
{
	static std::string	sUnactive = "δ����";
	static std::string	sDisconnected = "�Ͽ�״̬";
	static std::string	sConnected = "��ͨ״̬";
	static std::string	sLogin = "��¼�ɹ�";
	static std::string	sInitialized = "��ʼ����";
	static std::string	sWorking = "����������";
	static std::string	sUnknow = "����ʶ��״̬";

	switch( eStatus )
	{
	case ET_SS_UNACTIVE:
		return sUnactive;
	case ET_SS_DISCONNECTED:
		return sDisconnected;
	case ET_SS_CONNECTED:
		return sConnected;
	case ET_SS_LOGIN:
		return sLogin;
	case ET_SS_INITIALIZING:
		return sInitialized;
	case ET_SS_WORKING:
		return sWorking;
	default:
		return sUnknow;
	}
}

CTPWorkStatus&	CTPWorkStatus::operator= ( enum E_SS_Status eWorkStatus )
{
	CriticalLock	section( m_oLock );

	if( m_eWorkStatus != eWorkStatus )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPWorkStatus::operator=() : Exchange CTP Session Status [%s]->[%s]"
											, CastStatusStr(m_eWorkStatus).c_str(), CastStatusStr(eWorkStatus).c_str() );
				
		m_eWorkStatus = eWorkStatus;
	}

	return *this;
}


///< ----------------------------------------------------------------


CTPQuotation::CTPQuotation()
 : m_pCTPApi( NULL ), m_nCodeCount( 0 )
{
}

CTPQuotation::~CTPQuotation()
{
	Destroy();
}

CTPWorkStatus& CTPQuotation::GetWorkStatus()
{
	return m_oWorkStatus;
}

int CTPQuotation::Activate()
{
	if( true == Configuration::GetConfig().IsBroadcastModel() )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Activate() : ............ Broadcast Thread Activating............" );
		m_oWorkStatus = ET_SS_DISCONNECTED;								///< ����CTPQuotation�Ự��״̬

		if( 0 != SimpleTask::Activate() )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::Activate() : failed 2 activate broadcast thread..." );
			return -1;
		}

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Activate() : ............ Broadcast Thread Activated!............" );
		return 0;
	}

	if( GetWorkStatus() == ET_SS_UNACTIVE )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Activate() : ............ CTP Session Activating............" );
		CTPLinkConfig&		refConfig = Configuration::GetConfig().GetHQConfList();

		Destroy();
		m_pCTPApi = CThostFtdcMdApi::CreateFtdcMdApi();					///< ��CTP��DLL�����µ�api�ӿ�
		if( NULL == m_pCTPApi )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::Activate() : error occur while creating CTP control api" );
			return -3;
		}

		m_pCTPApi->RegisterSpi( this );									///< ��thisע��Ϊ�¼������ʵ��
		if( false == refConfig.RegisterServer( m_pCTPApi, NULL ) )		///< ע��CTP������Ҫ����������
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::Activate() : invalid front/name server address" );
			return -4;
		}

		m_pCTPApi->Init();												///< ʹ�ͻ��˿�ʼ�����鷢����������������
		m_oWorkStatus = ET_SS_DISCONNECTED;				///< ����CTPQuotation�Ự��״̬
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Activate() : ............ CTPQuotation Activated!.............." );
	}

	return 0;
}

int CTPQuotation::Destroy()
{
	if( m_pCTPApi )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Destroy() : ............ Destroying .............." );

		m_pCTPApi->RegisterSpi(NULL);
		m_pCTPApi->Release();
		m_pCTPApi = NULL;

		m_oWorkStatus = ET_SS_UNACTIVE;	///< ����CTPQuotation�Ự��״̬
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::Destroy() : ............ Destroyed! .............." );
	}

	return 0;
}

int CTPQuotation::LoadDataFile( std::string sFilePath, bool bEchoOnly )
{
	bool		bRet = QuotationSync::CTPSyncLoader::GetHandle().Init( sFilePath.c_str(), false );

	if( false == bRet )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuotation::LoadDataFile() : failed 2 open snap file : %s", sFilePath.c_str() );
		return -1;
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::LoadDataFile() : broadcasting quotation, errorcode = %d", bRet );

	while( true )
	{
		CThostFtdcDepthMarketDataField	oData = { 0 };

		if( QuotationSync::CTPSyncLoader::GetHandle().GetSnapData( oData ) <= 0 )
		{
			break;
		}

		if( true == bEchoOnly )
		{
			::printf( "%s\n", oData.InstrumentID );
		}
		else
		{
			OnRtnDepthMarketData( &oData );
			::Sleep( 50 );
		}
	}

	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::LoadDataFile() : End of Quotation File..." );

	return 0;
}

int CTPQuotation::Execute()
{
	DateTime		oTodayDate;
	char			pszTmpFile[128] = { 0 };			///< ׼��������������
	unsigned int	nNowTime = DateTime::Now().TimeToLong();

	oTodayDate.SetCurDateTime();
	m_oWorkStatus = ET_SS_INITIALIZING;					///< ����CTPQuotation�Ự��״̬
	if( nNowTime > 40000 && nNowTime < 180000 ) {
		::strcpy( pszTmpFile, "Quotation_day.dmp" );
	} else if( nNowTime > 0 && nNowTime < 40000 ) {
		oTodayDate -= (60*60*8);
		::strcpy( pszTmpFile, "Quotation_nite.dmp" );
	} else {
		::strcpy( pszTmpFile, "Quotation_nite.dmp" );
	}
	std::string		sDumpFile = GenFilePathByWeek( Configuration::GetConfig().GetDumpFolder().c_str(), pszTmpFile, oTodayDate.Now().DateToLong() );

	return LoadDataFile( sDumpFile, false );
}

int CTPQuotation::SubscribeQuotation()
{
	if( GetWorkStatus() == ET_SS_LOGIN )			///< ��¼�ɹ���ִ�ж��Ĳ���
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SubscribeQuotation() : [ATTENTION] - Quotation Is Subscribing................" );

		char				pszCodeList[1024*5][20] = { 0 };	///< ���Ĵ��뻺��
		char*				pszCodes[1024*5] = { NULL };		///< �����Ĵ����б�,�Ӹ��г�������������ȡ
		int					nRet = QuoCollector::GetCollector().GetSubscribeCodeList( pszCodeList, 1024*5 );

		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SubscribeQuotation() : Argument Num = %d.", nRet );
		for( int n = 0; n < nRet; n++ ) {
			pszCodes[n] = pszCodeList[n]+0;
		}

		if( nRet > 0 ) {
			QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SubscribeQuotation() : Creating ctp session\'s dump file ......" );

			DateTime		oTodayDate;
			char			pszTmpFile[128] = { 0 };			///< ׼��������������
			unsigned int	nNowTime = DateTime::Now().TimeToLong();

			oTodayDate.SetCurDateTime();
			if( nNowTime > 40000 && nNowTime < 180000 ) {
				::strcpy( pszTmpFile, "Quotation_day.dmp" );
			} else if( nNowTime > 0 && nNowTime < 40000 ) {
				oTodayDate -= (60*60*8);
				::strcpy( pszTmpFile, "Quotation_nite.dmp" );
			} else {
				::strcpy( pszTmpFile, "Quotation_nite.dmp" );
			}
			std::string		sDumpFile = GenFilePathByWeek( Configuration::GetConfig().GetDumpFolder().c_str(), pszTmpFile, oTodayDate.Now().DateToLong() );
			bool			bRet = QuotationSync::CTPSyncSaver::GetHandle().Init( sDumpFile.c_str(), DateTime::Now().DateToLong(), false );

			QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SubscribeQuotation() : dump file created, result = %d", bRet );
		}

		m_oWorkStatus = ET_SS_INITIALIZING;		///< ����CTPQuotation�Ự��״̬
		if( nRet > 0 ) { nRet = m_pCTPApi->SubscribeMarketData( pszCodes, nRet ); }
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SubscribeQuotation() : [ATTENTION] - Quotation Has Been Subscribed! errorcode(%d) !!!", nRet );
		return 0;
	}

	return -2;
}

void CTPQuotation::SendLoginRequest()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SendLoginRequest() : sending hq login message" );

	CThostFtdcReqUserLoginField	reqUserLogin = { 0 };

	strcpy( reqUserLogin.UserProductInfo,"�Ϻ�Ǭ¡�߿Ƽ����޹�˾" );
	strcpy( reqUserLogin.TradingDay, m_pCTPApi->GetTradingDay() );
	memcpy( reqUserLogin.Password, Configuration::GetConfig().GetHQConfList().m_sPswd.c_str(), Configuration::GetConfig().GetHQConfList().m_sPswd.length() );
	memcpy( reqUserLogin.UserID, Configuration::GetConfig().GetHQConfList().m_sUID.c_str(), Configuration::GetConfig().GetHQConfList().m_sUID.length() );
	memcpy( reqUserLogin.BrokerID, Configuration::GetConfig().GetHQConfList().m_sParticipant.c_str(), Configuration::GetConfig().GetHQConfList().m_sParticipant.length() );

	if( 0 == m_pCTPApi->ReqUserLogin( &reqUserLogin, 0 ) )
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::SendLoginRequest() : login message sended!" );
	}
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuotation::SendLoginRequest() : failed 2 send login message" );
	}
}

void CTPQuotation::OnFrontConnected()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::OnFrontConnected() : connection established." );

	m_oWorkStatus = ET_SS_CONNECTED;				///< ����CTPQuotation�Ự��״̬

    SendLoginRequest();
}

void CTPQuotation::OnFrontDisconnected( int nReason )
{
	const char*		pszInfo=nReason == 0x1001 ? "�����ʧ��" :
							nReason == 0x1002 ? "����дʧ��" :
							nReason == 0x2001 ? "����������ʱ" :
							nReason == 0x2002 ? "��������ʧ��" :
							nReason == 0x2003 ? "�յ�������" :
							"δ֪ԭ��";

	QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::OnFrontDisconnected() : connection disconnected, [reason:%d] [info:%s]", nReason, pszInfo );

	m_oWorkStatus = ET_SS_DISCONNECTED;			///< ����CTPQuotation�Ự��״̬
}

void CTPQuotation::OnHeartBeatWarning( int nTimeLapse )
{
	QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::OnHeartBeatWarning() : hb overtime, (%d)", nTimeLapse );
}

void CTPQuotation::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	CriticalLock	section( m_oLock );

	m_nCodeCount = 0;
	m_setRecvCode.clear();				///< ����յ��Ĵ��뼯�ϼ�¼

    if( pRspInfo->ErrorID != 0 )
	{
		// �˵�ʧ�ܣ��ͻ�������д�����
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuotation::OnRspUserLogin() : failed 2 login [Err=%d,ErrMsg=%s,RequestID=%d,Chain=%d]"
											, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

		m_oWorkStatus = ET_SS_CONNECTED;			///< ����CTPQuotation�Ự��״̬
        Sleep( 3000 );
        SendLoginRequest();
    }
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuotation::OnRspUserLogin() : succeed 2 login [RequestID=%d,Chain=%d]", nRequestID, bIsLast );
		m_oWorkStatus = ET_SS_LOGIN;				///< ����CTPQuotation�Ự��״̬
		SubscribeQuotation();
	}
}

unsigned int CTPQuotation::GetCodeCount()
{
	CriticalLock	section( m_oLock );

	return m_nCodeCount;
}

void CTPQuotation::OnRtnDepthMarketData( CThostFtdcDepthMarketDataField *pMarketData )
{
	if( NULL == pMarketData )
	{
		QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuotation::OnRtnDepthMarketData() : invalid market data pointer (NULL)" );
		return;
	}

	if( false == Configuration::GetConfig().IsBroadcastModel() )
	{
		QuotationSync::CTPSyncSaver::GetHandle().SaveSnapData( *pMarketData );
	}

	///< �ж��Ƿ�����ȫ������(���յ��Ĵ����Ƿ����ظ�Ϊ�ж�����)
	bool	bInitializing = (enum E_SS_Status)m_oWorkStatus != ET_SS_WORKING;
	if( true == bInitializing )
	{
		CriticalLock	section( m_oLock );

		if( m_setRecvCode.find( pMarketData->InstrumentID ) == m_setRecvCode.end() )
		{
			m_setRecvCode.insert( pMarketData->InstrumentID );
			m_nCodeCount = m_setRecvCode.size();
		}
		else
		{
			m_oWorkStatus = ET_SS_WORKING;	///< �յ��ظ����룬ȫ��������������
			bInitializing = true;
		}
	}

	FlushQuotation( pMarketData, bInitializing );
}

void CTPQuotation::FlushQuotation( CThostFtdcDepthMarketDataField* pQuotationData, bool bInitialize )
{
	double							dRate = 1.;				///< �Ŵ���
	int								nSerial = 0;			///< ��Ʒ����������ֵ
	tagSHFutureReferenceData_LF110	tagName = { 0 };		///< ��Ʒ������Ϣ�ṹ
	tagSHFutureSnapData_HF112		tagSnapHF = { 0 };		///< �����������
	tagSHFutureSnapData_LF111		tagSnapLF = { 0 };		///< �����������
	tagSHFutureSnapBuySell_HF113	tagSnapBS = { 0 };		///< ��λ��Ϣ
	tagSHFutureMarketStatus_HF109	tagStatus = { 0 };		///< �г�״̬��Ϣ
	unsigned int					nSnapTradingDate = 0;	///< ���ս�������

	::strncpy( tagName.Code, pQuotationData->InstrumentID, sizeof(tagName.Code) );
	::memcpy( tagSnapHF.Code, pQuotationData->InstrumentID, sizeof(tagSnapHF.Code) );
	::memcpy( tagSnapLF.Code, pQuotationData->InstrumentID, sizeof(tagSnapLF.Code) );
	::memcpy( tagSnapBS.Code, pQuotationData->InstrumentID, sizeof(tagSnapBS.Code) );

	if( (nSerial=QuoCollector::GetCollector()->OnQuery( 110, (char*)&tagName, sizeof(tagName) )) <= 0 )
	{
		return;
	}

	dRate = CTPQuoImage::GetRate( tagName.Kind );

	if( true == bInitialize ) {	///< ��ʼ������
		tagSnapLF.PreOpenInterest = pQuotationData->PreOpenInterest*dRate+0.5;
	}
	if( pQuotationData->UpperLimitPrice > 0 ) {
		tagSnapLF.UpperPrice = pQuotationData->UpperLimitPrice*dRate+0.5;
	}
	if( pQuotationData->LowerLimitPrice > 0 ) {
		tagSnapLF.LowerPrice = pQuotationData->LowerLimitPrice*dRate+0.5;
	}

	tagSnapLF.Open = pQuotationData->OpenPrice*dRate+0.5;
	tagSnapLF.Close = pQuotationData->ClosePrice*dRate+0.5;
	tagSnapLF.PreClose = pQuotationData->PreClosePrice*dRate+0.5;
	tagSnapLF.PreSettlePrice = pQuotationData->PreSettlementPrice*dRate+0.5;
	tagSnapLF.SettlePrice = pQuotationData->SettlementPrice*dRate+0.5;
	tagSnapLF.PreOpenInterest = pQuotationData->PreOpenInterest;

	tagSnapHF.High = pQuotationData->HighestPrice*dRate+0.5;
	tagSnapHF.Low = pQuotationData->LowestPrice*dRate+0.5;
	tagSnapHF.Now = pQuotationData->LastPrice*dRate+0.5;
	tagSnapHF.Position = pQuotationData->OpenInterest;
	tagSnapHF.Volume = pQuotationData->Volume;

//	if( EV_MK_ZZ == eMkID )		///< ֣���г��ĳɽ�������⴦�� = ��� * ��Լ��λ
//		tagSnapHF.Amount = pQuotationData->Turnover * refNameTable.ContractMult;

	tagSnapHF.Amount = pQuotationData->Turnover;
	tagSnapBS.Buy[0].Price = pQuotationData->BidPrice1*dRate+0.5;
	tagSnapBS.Buy[0].Volume = pQuotationData->BidVolume1;
	tagSnapBS.Sell[0].Price = pQuotationData->AskPrice1*dRate+0.5;
	tagSnapBS.Sell[0].Volume = pQuotationData->AskVolume1;
	tagSnapBS.Buy[1].Price = pQuotationData->BidPrice2*dRate+0.5;
	tagSnapBS.Buy[1].Volume = pQuotationData->BidVolume2;
	tagSnapBS.Sell[1].Price = pQuotationData->AskPrice2*dRate+0.5;
	tagSnapBS.Sell[1].Volume = pQuotationData->AskVolume2;
	tagSnapBS.Buy[2].Price = pQuotationData->BidPrice3*dRate+0.5;
	tagSnapBS.Buy[2].Volume = pQuotationData->BidVolume3;
	tagSnapBS.Sell[2].Price = pQuotationData->AskPrice3*dRate+0.5;
	tagSnapBS.Sell[2].Volume = pQuotationData->AskVolume3;
	tagSnapBS.Buy[3].Price = pQuotationData->BidPrice4*dRate+0.5;
	tagSnapBS.Buy[3].Volume = pQuotationData->BidVolume4;
	tagSnapBS.Sell[3].Price = pQuotationData->AskPrice4*dRate+0.5;
	tagSnapBS.Sell[3].Volume = pQuotationData->AskVolume4;
	tagSnapBS.Buy[4].Price = pQuotationData->BidPrice5*dRate+0.5;
	tagSnapBS.Buy[4].Volume = pQuotationData->BidVolume5;
	tagSnapBS.Sell[4].Price = pQuotationData->AskPrice5*dRate+0.5;
	tagSnapBS.Sell[4].Volume = pQuotationData->AskVolume5;

	char	pszTmpDate[12] = { 0 };
	::memcpy( pszTmpDate, pQuotationData->UpdateTime, sizeof(TThostFtdcTimeType) );
	pszTmpDate[2] = 0;
	pszTmpDate[5] = 0;
	pszTmpDate[8] = 0;
	int		nSnapUpdateTime = atol(pszTmpDate);
	nSnapUpdateTime = nSnapUpdateTime*100+atol(&pszTmpDate[3]);
	nSnapUpdateTime = nSnapUpdateTime*100+atol(&pszTmpDate[6]);
	if( (nSnapTradingDate=::atol( pQuotationData->TradingDay )) >= 0 && nSnapUpdateTime > 0 )
	{	///< ��������+ʱ��
		::strcpy( tagStatus.Key, "mkstatus" );
		tagStatus.MarketStatus = 1;
		tagStatus.MarketTime = nSnapUpdateTime;
		QuoCollector::GetCollector()->OnData( 109, (char*)&tagStatus, sizeof(tagStatus), false );
	}

	QuoCollector::GetCollector()->OnData( 111, (char*)&tagSnapLF, sizeof(tagSnapLF), false );
	QuoCollector::GetCollector()->OnData( 112, (char*)&tagSnapHF, sizeof(tagSnapHF), false );
	QuoCollector::GetCollector()->OnData( 113, (char*)&tagSnapBS, sizeof(tagSnapBS), false );
}

void CTPQuotation::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuotation::OnRspError() : [%s] ErrorCode=[%d], ErrorMsg=[%s],RequestID=[%d], Chain=[%d]"
										, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );
}






