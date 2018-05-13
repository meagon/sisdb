
//******************************************************
// Copyright (C) 2018, Martin <seerlinecoin@gmail.com>
//*******************************************************

#ifndef _STS_TABLE_H
#define _STS_TABLE_H

#include "dict.h"
#include "sts_fields.h"
#include "sts_zip.h"
#include "lw_map.h"
#include "lw_list.h"

/////////////////////////////////////////////////////////
//  ���ݿ����ݲ���ģʽ
/////////////////////////////////////////////////////////
#define STS_INSERT_PUSH        0  // �����ж�ֱ��׷��
#define STS_INSERT_TS_CHECK    1  // ���ʱ��ڵ��ظ��͸����ϵ����ݣ����ظ��ͷŶ�Ӧλ��
#define STS_INSERT_MUL_CHECK   2  // ����3���������ݲ���ȷ�����ݵ�׼ȷ�ԣ��ݲ���

/////////////////////////////////////////////////////////
//  ���ݸ��Ƕ��� 
/////////////////////////////////////////////////////////
#define STS_DATA_ZIP     'R'   // ���濪ʼΪ����
#define STS_DATA_BIN     'B'   // ���濪ʼΪ����
#define STS_DATA_STRING  'S'   // ���濪ʼΪ����
#define STS_DATA_JSON    '{'   // ֱ�Ӵ�����
#define STS_DATA_ARRAY   '['   // ֱ�Ӵ�����

#pragma pack(push,1)

typedef struct sts_table_control{
	uint32_t version;      // ���ݱ��İ汾��time_t��ʽ
	uint32_t limit_rows;   // ÿ��collection������¼��
	uint8_t  zip_mode;     // ѹ��ʱ�������ݴ��һ����
	uint8_t  insert_mode;  // �������ݷ�ʽ
}sts_table_control;

typedef struct sts_table{
	sds name;            //��������
	sts_table_control control;       // �����ƶ���
	s_string_list  *field_name;      // ��˳���ŵ�����
	s_map_pointer  *field_map;       // �ֶζ����ֵ�������ֶ����洢���ֶ��ڴ�飬ָ��sts_field_unit
	s_map_pointer  *collect_map;     // ���ݶ����ֵ��������Ʊ���洢�������ڴ�飬ָ��sts_collect_unit
}sts_table;

#pragma pack(pop)

sts_table *create_sts_table(const char *name_, const char *command);  //commandΪһ��json��ʽ�ֶζ���
// commandΪjson����
//�û������command�йؼ��ֵĶ������£�
//�ֶζ��壺  "fields":  []
//��¼�����ƣ�"limits":  0 ��¼������  
//ѹ����ʽ��  "zipmode":  0
//���뷽ʽ��  "insert":  1 �������ݷ�ʽ�����limitsΪ1���������޸ĵ�һ����¼

void destroy_sts_table(sts_table *);  //ɾ��һ����
void clear_sts_table(sts_table *);    //����һ��������������
//�����ݿ�ĸ�����������
void sts_table_set_ver(sts_table *, uint32_t);  // time_t��ʽ
void sts_table_set_limit_rows(sts_table *, uint32_t); // 0 -- ������  1 -- ֻ�������µ�һ��  n 
void sts_table_set_zip_mode(sts_table *, uint8_t); // 0 -- ��ѹ�� 1 2
void sts_table_set_insert_mode(sts_table *, uint8_t); // 1 -- �жϺ��޸� 0 2

void sts_table_set_fields(sts_table *, const char *command); //commandΪһ��json��ʽ�ֶζ���
//��ȡ���ݿ�ĸ���ֵ
sts_field_unit *sts_table_get_field(sts_table *tb_, const char *name_);
int sts_table_get_fields_size(sts_table *);
uint64 sts_table_get_times(sts_table *, void *); // ��ȡʱ������,Ĭ��Ϊ��һ���ֶΣ�����һ���ֶβ����ϱ�׼��������
sds sts_table_get_string_m(sts_table *, void *, const char *name_);
//�õ���¼�ĳ���
//ȡ���ݺ�д����
void sts_table_update(sts_table *, const char *key_, sds value_); 
//�޸����ݣ�key_Ϊ��Ʊ������г���ţ�value_Ϊ�����ƽṹ�����ݻ�json����
sds sts_table_get_m(sts_table *, const char *key_, const char *command);  //����������Ҫ�ͷ�

// commandΪjson����
//�����д���Ϊkey�����ݣ�keyΪ*��ʾ���й�Ʊ���ݣ���command�������ݷ�Χ���ֶη�Χ
//�û������command�йؼ��ֵĶ������£�
//�������ݸ�ʽ��"format":"json" --> STS_DATA_JSON
//						 "array" --> STS_DATA_ARRAY
//						 "bin" --> STS_DATA_BIN  ----> Ĭ��
//					     "string" --> STS_DATA_STRING
//						 "zip" --> STS_DATA_ZIP
//�ֶΣ�    "fields":  "time,close,vol,name" ��ʾһ��4���ֶ�  
//                      ��,*---->��ʾȫ���ֶ�
//---------<��������û�б�ʾȫ������>--------
//���ݷ�Χ��"search":   min,max ��ʱ������ȡ����
//						count(��max���⣬����ʾ��󣬸���ʾ��ǰ),
//						forceΪ0��ʾ��ʵ��ȡ��Ϊ1�������ݾ�ȡminǰһ�����ݣ�
//���ݷ�Χ��"range":    start��stop ����¼��ȡ���� 0��-1-->��ʾȫ������
//						count(��stop���⣬����ʾ��󣬸���ʾ��ǰ),

int sts_table_delete(sts_table *, const char *key_, const char *command);// commandΪjson����
//ɾ��
//�û������command�йؼ��ֵĶ������£�
//���ݷ�Χ��"search":   min,max ��ʱ������ȡ����
//						count(��max���⣬����ʾ��󣬸���ʾ��ǰ),
//���ݷ�Χ��"range":    start��stop ����¼��ȡ���� 0��-1-->��ʾȫ������
//						count(��stop���⣬����ʾ��󣬸���ʾ��ǰ),

//     

#endif  /* _STS_TABLE_H */