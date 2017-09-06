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
		::printf( "合约代码,交易所代码,合约名称,合约在交易所内代码,产品代码,产品类型,交割年份,交割月,市价单最大下单量,市价单最小下单量,限价单最大下单量,限价单最小下单量,合约数量乘数,最小变动价位,\
创建日,上市日,到期日,开始交割日,结束交割日,合约生命周期状态,当前是否交易,持仓类型,持仓日期类型,多头保证金率,空头保证金率,是否使用大额单边保证金算法,基础商品代码,执行价,期权类型,\
合约基础商品乘数,组合类型\n" );
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
	m_mapRate.clear();													///< 清空放大倍数映射表
	m_mapKind.clear();

	if( false == Configuration::GetConfig().IsBroadcastModel() )
	{
		FreeApi();///< 清理上下文 && 创建api控制对象
		if( NULL == (m_pTraderApi=CThostFtdcTraderApi::CreateFtdcTraderApi()) )
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : error occur while creating CTP trade control api" );
			return -1;
		}

		char		pszTmpFile[1024] = { 0 };							///< 准备请求的静态数据落盘
		::sprintf( pszTmpFile, "Trade_%u_%u.dmp", DateTime::Now().DateToLong(), DateTime::Now().TimeToLong() );
		m_oDataRecorder.OpenFile( JoinPath( Configuration::GetConfig().GetDumpFolder(), pszTmpFile ).c_str(), true );					///< 准备好落盘文件的句柄

		m_pTraderApi->RegisterSpi( this );								///< 将this注册为事件处理的实例
		if( false == Configuration::GetConfig().GetTrdConfList().RegisterServer( NULL, m_pTraderApi ) )///< 注册CTP链接需要的网络配置
		{
			QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : invalid front/name server address" );
			return -2;
		}

		m_pTraderApi->Init();											///< 使客户端开始与行情发布服务器建立连接
		for( int nLoop = 0; false == m_bIsResponded; nLoop++ )			///< 等待请求响应结束
		{
			SimpleThread::Sleep( 1000 );
			if( nLoop > 60 * 3 ) {
				QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::FreshCache() : overtime (>=3 min)" );
				return -3;
			}
		}

		FreeApi();														///< 释放api+落盘文件句柄，结束请求
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

	///< 配置分类信息
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
			refKind.MarketPeriods[0][0] = 21*60;			///< 第一段，取夜盘的时段的最大范围
			refKind.MarketPeriods[0][1] = 23*60+30;
			refKind.MarketPeriods[1][0] = 9*60;				///< 第二段
			refKind.MarketPeriods[1][1] = 10*60+15;
			refKind.MarketPeriods[2][0] = 10*60+30;			///< 第三段
			refKind.MarketPeriods[2][1] = 11*60+30;
			refKind.MarketPeriods[3][0] = 13*60+30;			///< 第四段
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
	m_nTrdReqID = 0;						///< 重置请求ID
	m_bIsResponded = false;					///< 重置响应完成标识
	m_oDataRecorder.CloseFile();			///< 关闭落盘文件句柄

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

	m_nTrdReqID = 0;						///< 重置请求ID
	m_bIsResponded = true;					///< 停止等待数据返回
	m_mapBasicData.clear();					///< 清空所有缓存数据
}

void CTPQuoImage::SendLoginRequest()
{
	QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::SendLoginRequest() : sending trd login message" );

	CThostFtdcReqUserLoginField	reqUserLogin = { 0 };

	memcpy( reqUserLogin.BrokerID, Configuration::GetConfig().GetTrdConfList().m_sParticipant.c_str(), Configuration::GetConfig().GetTrdConfList().m_sParticipant.length() );
	memcpy( reqUserLogin.UserID, Configuration::GetConfig().GetTrdConfList().m_sUID.c_str(), Configuration::GetConfig().GetTrdConfList().m_sUID.length() );
	memcpy( reqUserLogin.Password, Configuration::GetConfig().GetTrdConfList().m_sPswd.c_str(), Configuration::GetConfig().GetTrdConfList().m_sPswd.length() );
	strcpy( reqUserLogin.UserProductInfo,"上海乾隆高科技有限公司" );
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
	const char*		pszInfo=nReason == 0x1001 ? "网络读失败" :
							nReason == 0x1002 ? "网络写失败" :
							nReason == 0x2001 ? "接收心跳超时" :
							nReason == 0x2002 ? "发送心跳失败" :
							nReason == 0x2003 ? "收到错误报文" :
							"未知原因";

	BreakOutWaitingResponse();

	QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnFrontDisconnected() : connection disconnected, [reason:%d] [info:%s]", nReason, pszInfo );
}

void CTPQuoImage::OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
    if( pRspInfo->ErrorID != 0 )
	{
		// 端登失败，客户端需进行错误处理
		QuoCollector::GetCollector()->OnLog( TLV_WARN, "CTPQuoImage::OnRspUserLogin() : failed 2 login [Err=%d,ErrMsg=%s,RequestID=%d,Chain=%d]"
											, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

		SimpleThread::Sleep( 1000*6 );
        SendLoginRequest();
    }
	else
	{
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnRspUserLogin() : succeed 2 login [RequestID=%d,Chain=%d]", nRequestID, bIsLast );

		///请求查询合约
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
		if( pRspInfo->ErrorID != 0 )		///< 查询失败
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
		{	///< 判断为是否需要过滤的商品
			CThostFtdcInstrumentField&		refSnap = *pInstrument;						///< 交易请求接口返回结构
			CriticalLock					section( m_oLock );
			tagSHFutureReferenceData_LF110	tagName = { 0 };							///< 商品基础信息结构
			tagSHFutureSnapData_HF112		tagSnapHF = { 0 };							///< 高速行情快照
			tagSHFutureSnapData_LF111		tagSnapLF = { 0 };							///< 低速行情快照
			tagSHFutureSnapBuySell_HF113	tagSnapBS = { 0 };							///< 档位信息

			::strncpy( tagName.Code, refSnap.InstrumentID, sizeof(tagName.Code) );		///< 商品代码
			::memcpy( tagSnapHF.Code, refSnap.InstrumentID, sizeof(tagSnapHF.Code) );	///< 商品代码
			::memcpy( tagSnapLF.Code, refSnap.InstrumentID, sizeof(tagSnapLF.Code) );	///< 商品代码
			::memcpy( tagSnapBS.Code, refSnap.InstrumentID, sizeof(tagSnapBS.Code) );	///< 商品代码

			T_MAP_KIND::iterator it = m_mapKind.find( refSnap.UnderlyingInstrID );
			if( it != m_mapKind.end() )
			{
				tagName.Kind = ::atoi( it->second.Key );
				tagName.DeliveryDate = ::atol(refSnap.StartDelivDate);						///< 交割日(YYYYMMDD)
				tagName.StartDate = ::atol(refSnap.OpenDate);								///< 首个交易日(YYYYMMDD)
				tagName.EndDate = ::atol(refSnap.ExpireDate);								///< 最后交易日(YYYYMMDD), 即 到期日
				tagName.ExpireDate = tagName.EndDate;										///< 到期日(YYYYMMDD)
				::strncpy( tagName.Name, refSnap.InstrumentName, sizeof(tagName.Code) );	///< 商品名称
				tagName.XqPrice = refSnap.StrikePrice*GetRate(tagName.Kind)+0.5;		///< 行权价格(精确到厘) //[*放大倍数] 

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
	{	///< 最后一条
		CriticalLock	section( m_oLock );
		m_bIsResponded = true;				///< 停止等待数据返回(请求返回完成[OK])
		QuoCollector::GetCollector()->OnLog( TLV_INFO, "CTPQuoImage::OnRspQryInstrument() : [DONE] basic data freshed! size=%d", m_mapBasicData.size() );
	}
}

void CTPQuoImage::OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast )
{
	QuoCollector::GetCollector()->OnLog( TLV_ERROR, "CTPQuoImage::OnRspError() : ErrorCode=[%d], ErrorMsg=[%s],RequestID=[%d], Chain=[%d]"
										, pRspInfo->ErrorID, pRspInfo->ErrorMsg, nRequestID, bIsLast );

	BreakOutWaitingResponse();
}








