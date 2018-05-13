
//******************************************************
// Copyright (C) 2018, Martin <seerlinecoin@gmail.com>
//*******************************************************

#ifndef _STS_ZIP_H
#define _STS_ZIP_H

/////////////////////////////////////////////////////////
//  ѹ�����뷽ʽ���� ����4�ַ�ʽ
/////////////////////////////////////////////////////////
#define STS_ENCODEING_SRC  0x000  // ���ۺ�ʱ����ѹ��  SRC
#define STS_ENCODEING_ROW  0x001  // ֻ�͵�ǰ��¼ͬ��������ѹ�� ROW
#define STS_ENCODEING_COL  0x010  // ֻ����һ����¼ͬһ������ѹ�� COL
#define STS_ENCODEING_ALL  0x011  // ����һ����¼ͬ��������Ϊ�ο���ѹ����ǰ��¼ͬ��������  ALL
#define STS_ENCODEING_STR  0x100  // �Լ�ѹ���Լ���ר�����޹����ַ���ѹ��  STR
#define STS_ENCODEING_COD  0x101  // �Լ�ѹ���Լ���ר�������ֺ���ĸ��ѹ��  COD
#define STS_ENCODEING_O22  0x110  // ����
#define STS_ENCODEING_O33  0x111  // ����

/////////////////////////////////////////////////////////
//  ����ѹ��ģʽ
/////////////////////////////////////////////////////////
#define STS_ZIP_NO       0  // ��ѹ��
#define STS_ZIP_ONE      1  // 1����¼һ����
#define STS_ZIP_BLOCK    2  // �������ݿ��Сѹ������,STS_ZIP_SIZE
#define STS_ZIP_SIZE     (4 * 1024)  //ѹ������󳤶ȣ�

//ѹ�����ݽṹ
//��һ����Ϊ�ֶζ��壬�����ֶθ������ֶ���+�ֶ�����
//........��һ���ֿ��Ե������䣬Ҳ���Ժ�������һ���䣬ȡ��������ķ�ʽ��Ĭ����һ���䣬
//�ڶ�����Ϊ���ݿ飬��һ���ַ���ʾ�Ƿ�ѹ��
//                  R...��ʾѹ��
//                  B...��ʾ�����Ʋ�����
//                  {...��ʾJSON���ݸ�ʽ
//					[...��ʾ�������ݸ�ʽ
//                  S...��ʾ�ַ���
//    �ڶ�����������󲻳���STS_ZIP_SIZE�����һ����¼�ͳ����ͷֿ飬��¼��д0���ֿ�󣬱��뱣֤һ�鴫���ٴ���һ����¼��
//��־��д��д��¼������250=250 ................ 0 -- 250
//						251+N=250+N+1 .......... 251 -- 506
//						252+N=(250+256)+N+1..... 507 -- 762
//						253+N=(250+2*256)+N+1... 763 -- 1018
//						254+N=(250+3*256)+N+1... 1019 -- 1274
//                      255�����2λ=65535��4K������˵������1λһ������Ҳ���㹻��
//��ǰ���ݿ���ʼѹ������ֵ����ѹ����---- 
//��Ȼ����ʵ�ʵ����������洢һ�����ݰ������ݣ�
//��ǰ���ݿ������������һ����¼��ѹ����ֵ����ѹ�� ---- �ÿ�������紫���ǷǱ�Ҫ��
//˳�򱣴�ѹ������

// ��ѹ�����У��ֶεĶ���
typedef struct sts_fields_zip{
	unsigned type : 5;      // ��������  һ��32����������
	unsigned encoding : 3;  // 8�ֱ��뷽ʽ
}sts_fields_zip;
// ����ֶ�����STS_FIELD_PRC����STS_FIELD_VOL����ô�ֶζ���������ľ�������Ľṹ
typedef struct sts_fields_zip_other{
	unsigned prc_decimal : 4;  // �۸���С����
	unsigned vol_zoomout : 4;  // �ɽ����Ŵ���
}sts_fields_zip_other;


#endif  /* _STS_ZIP_H */