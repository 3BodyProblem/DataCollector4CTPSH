#ifndef __CTP_QUOTATION_PROTOCAL_CTP_SH_H__
#define	__CTP_QUOTATION_PROTOCAL_CTP_SH_H__
#pragma pack(1)


typedef struct
{
	char						Key[20];					///< ������ֵ
	unsigned int				MarketID;					///< �г����
	unsigned int				MarketDate;					///< �г�����
	unsigned int				KindCount;					///< �������
	unsigned int				WareCount;					///< ��Ʒ����
} tagSHFutureMarketInfo_LF107;


typedef struct
{
	char						Key[20];					///< ������ֵ
	char						KindName[64];				///< ��������
	unsigned int				PriceRate;					///< �۸�Ŵ���[10�Ķ��ٴη�]
	unsigned int				LotSize;					///< һ�ֵ��ڼ��ź�Լ
	unsigned int				LotFactor;					///< ���֡�����
	unsigned int				PriceTick;					///< ��С�䶯��λ
	unsigned int				ContractMult;				///< ��Լ����
	char						UnderlyingCode[20];			///< ���֤ȯ����
	unsigned char				DerivativeType;				///< ����Ʒ���ͣ���ʽ�Ϲ�0/�Ϲ�1 ŷʽ�Ϲ�2/�Ϲ�3
	unsigned int				PeriodsCount;				///< ����ʱ����Ϣ�б���
	unsigned int				MarketPeriods[10][2];		///< ����ʱ��������Ϣ�б�
} tagSHFutureKindDetail_LF108;


typedef struct
{
	char						Key[20];					///< ������ֵ
	unsigned int				MarketTime;					///< �г�ʱ��
	unsigned char				MarketStatus;				///< �г�״̬[0��ʼ�� 1������]
} tagSHFutureMarketStatus_HF109;


typedef struct
{
	char						Code[20];					///< ��Լ����
	char						Name[64];					///< ��Լ����
	unsigned int				Kind;						///< ֤ȯ����
	unsigned int				XqPrice;					///< ��Ȩ�۸�[*�Ŵ���]
	unsigned int				StartDate;					///< �׸�������(YYYYMMDD)
	unsigned int				EndDate;					///< �������(YYYYMMDD)
	unsigned int				DeliveryDate;				///< ������(YYYYMMDD)
	unsigned int				ExpireDate;					///< ������(YYYYMMDD)
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









