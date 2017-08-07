#ifndef __CTP_QUOTATION_PROTOCAL_CTP_DL_H__
#define	__CTP_QUOTATION_PROTOCAL_CTP_DL_H__
#pragma pack(1)


typedef struct
{
	char						Key[20];					///< ������ֵ
	unsigned int				MarketID;					///< �г����
	unsigned int				KindCount;					///< �������
	unsigned int				WareCount;					///< ��Ʒ����
	unsigned int				PeriodsCount;				///< ����ʱ����Ϣ�б���
	unsigned int				MarketPeriods[8][2];		///< ����ʱ��������Ϣ�б�
} tagSHFutureMarketInfo_LF107;


typedef struct
{
	char						Key[20];					///< ������ֵ
	char						KindName[64];				///< ��������
	unsigned int				PriceRate;					///< �۸�Ŵ���[10�Ķ��ٴη�]
	unsigned int				LotFactor;					///< ���֡�����
	unsigned int				WareCount;					///< �÷������Ʒ����
} tagSHFutureKindDetail_LF108;


typedef struct
{
	char						Key[20];					///< ������ֵ
	unsigned int				MarketDate;					///< �г�����
	unsigned int				MarketTime;					///< �г�ʱ��
	unsigned char				MarketStatus;				///< �г�״̬[0��ʼ�� 1������]
} tagSHFutureMarketStatus_HF109;


typedef struct
{
	char						Code[20];					///< ��Լ����
	char						Name[64];					///< ��Լ����
	unsigned char				Kind;						///< ֤ȯ����
	unsigned char				DerivativeType;				///< ����Ʒ���ͣ�ŷʽ��ʽ+�Ϲ��Ϲ�
	unsigned int				LotSize;					///< һ�ֵ��ڼ��ź�Լ
	char						UnderlyingCode[20];			///< ���֤ȯ����
	unsigned int				ContractMult;				///< ��Լ����
	unsigned int				XqPrice;					///< ��Ȩ�۸�[*�Ŵ���]
	unsigned int				StartDate;					///< �׸�������(YYYYMMDD)
	unsigned int				EndDate;					///< �������(YYYYMMDD)
	unsigned int				XqDate;						///< ��Ȩ��(YYYYMM)
	unsigned int				DeliveryDate;				///< ������(YYYYMMDD)
	unsigned int				ExpireDate;					///< ������(YYYYMMDD)
	unsigned short				TypePeriodIdx;				///< ���ཻ��ʱ���λ��
	unsigned char				EarlyNightFlag;             ///< ����orҹ�̱�־ 1������ 2��ҹ�� 
	unsigned int				PriceTick;					///< ��С�䶯��λ
} tagSHFutureReferenceData_LF110;


typedef struct
{
	char						Code[20];					///< ��Լ����
	unsigned int				Open;						///< ���̼�[*�Ŵ���]
	unsigned int				Close;						///< ���ռ�[*�Ŵ���]
	unsigned int				PreClose;					///< ���ռ�[*�Ŵ���]
	unsigned int				UpperPrice;					///< ������ͣ�۸�[*�Ŵ���], 0��ʾ������
	unsigned int				LowerPrice;					///< ���յ�ͣ�۸�[*�Ŵ���], 0��ʾ������
	unsigned int				SettlePrice;				///< ����[*�Ŵ���]
	unsigned int				PreSettlePrice;				///< ��Լ���[*�Ŵ���]
	unsigned __int64			PreOpenInterest;			///< ���ճֲ���(��)
} tagSHFutureSnapData_LF111;


typedef struct
{
	char						Code[20];					///< ��Լ����
	unsigned int				Now;						///< ���¼�[*�Ŵ���]
	unsigned int				High;						///< ��߼�[*�Ŵ���]
	unsigned int				Low;						///< ��ͼ�[*�Ŵ���]
	double						Amount;						///< �ܳɽ����[Ԫ]
	unsigned __int64			Volume;						///< �ܳɽ���[��/��]
	unsigned __int64			Position;					///< �ֲ���
} tagSHFutureSnapData_HF112;


typedef struct
{
	unsigned int				Price;						///< ί�м۸�[* �Ŵ���]
	unsigned __int64			Volume;						///< ί����[��]
} tagSHFutureBuySellItem;


typedef struct
{
	char						Code[20];					///< ��Լ����
	tagSHFutureBuySellItem		Buy[5];						///< ���嵵
	tagSHFutureBuySellItem		Sell[5];					///< ���嵵
} tagSHFutureSnapBuySell_HF113;



#pragma pack()
#endif









