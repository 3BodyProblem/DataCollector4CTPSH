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


typedef std::map<unsigned int,unsigned int>					T_MAP_RATE;			///< 放大倍数映射表[商品分类,商品放大倍数]
typedef std::map<std::string,CThostFtdcInstrumentField>		T_MAP_BASEDATA;		///< 基础数据缓存


/**
 * @class			CTPQuoImage
 * @brief			各市场的基础数据构建类
 * @detail			用于在初始化的阶段，从交易服务器取得码表等静态数据信息；
					用于生成码表，并以此为判断行情是否转发的依据。
 * @note			已经根据配置文件中的"期货/权"分类选择的设定，过滤了不需要的分类商品的信息
 * @author			barry
 */
class CTPQuoImage : public CThostFtdcTraderSpi
{
public:
	CTPQuoImage();

	/**
	 * @brief				更新基础数据
	 * @return				返回取到的商品码表数量
	 */
	int						FreshCache();

	/**
	 * @brief				释放api资源
	 * @return				>=0			成功
							<0			失败
	 */
	int						FreeApi();

	/**
	 * @brief				取得能订阅的商品
	 * @param[out]			pszCodeList	商品列表
	 * @param[in]			nListSize	商品列表长度
	 * @return				返回个数
	 */
	int						GetSubscribeCodeList( char (&pszCodeList)[1024*5][20], unsigned int nListSize );

	/**
	 * @brief				引取码表缓存
	 */
	operator T_MAP_BASEDATA&();

	/**
	 * @brief				获取放大倍数
	 */
	static int				GetRate( unsigned int nKind );

protected:///< 自有方法函数
	/**
	 * @brief				发送登录请求包
	 */
    void					SendLoginRequest();

	/**
	 * @brief				建立推送基础数据
	 */
	void					BuildBasicData();

	/**
	 * @brief				跳出响应等待
	 */
	void					BreakOutWaitingResponse();

	/**
	 * @brief				根据代码判断商品类型
	 */
	int						JudgeKindFromSecurityID( char* pszCode, unsigned int nCodeLen = 9 ) const;

	/**
	 * @brief				从商品代码中取出执行日期
	 */
	int						ParseExerciseDateFromCode( char (&Code)[20] ) const;

protected:///< CThostFtdcTraderSpi的回调接口
	void					OnFrontConnected();
	void					OnFrontDisconnected( int nReason );
	void					OnHeartBeatWarning( int nTimeLapse );
	void					OnRspError( CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );
	void					OnRspUserLogin( CThostFtdcRspUserLoginField *pRspUserLogin, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );
	void					OnRspQryInstrument( CThostFtdcInstrumentField *pInstrument, CThostFtdcRspInfoField *pRspInfo, int nRequestID, bool bIsLast );

protected:
	bool					m_bIsResponded;				///< 返回完成
	unsigned short			m_nTrdReqID;				///< 交易api请求ID
	CThostFtdcTraderApi*	m_pTraderApi;				///< 交易模块接口
	static T_MAP_RATE		m_mapRate;					///< 各分类的放大倍数
	T_MAP_BASEDATA			m_mapBasicData;				///< 市场商品基础数据集合
	CriticalObject			m_oLock;					///< 临界区对象
};



#endif






