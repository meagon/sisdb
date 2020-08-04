﻿#ifndef _SISDB_CLIENT_H
#define _SISDB_CLIENT_H

#include "sis_method.h"
#include "sis_net.h"
#include "sis_list.h"
#include "sis_map.h"

typedef struct s_sisdb_client_ask {
	char       source[16];     // 数据来源标识

    bool       issub;          // 是否订阅
	s_sis_sds  cmd;            // 请求的参数
	s_sis_sds  key;            // 请求的key
	s_sis_sds  val;            // 请求的val

    void              *cb_source;
    sis_method_define *cb_sub_start;
    sis_method_define *cb_sub_realtime;
    sis_method_define *cb_sub_stop;

	sis_method_define *cb_reply;  // 回调的数据
}s_sisdb_client_ask;

#define SIS_CLI_STATUS_INIT  0   // 初始化完成
#define SIS_CLI_STATUS_AUTH  1   // 进行验证
#define SIS_CLI_STATUS_WORK  2   // 正常工作
#define SIS_CLI_STATUS_STOP  3   // 断开
#define SIS_CLI_STATUS_EXIT  4   // 退出
// 客户的结构体
typedef struct s_sisdb_client_cxt
{
	int  status;

    int                 cid;
    s_sis_url           url_cli;
    s_sis_net_class    *session;

    bool auth;
	char username[32]; 
	char password[32];

    uint32 ask_sno;      // 请求序列号    

    s_sis_sds               info;  // 返回的错误信息
	// 订阅的数据列表
    s_sis_map_pointer      *asks;   // 订阅的信息 断线后需要重新发送订阅信息 以key为索引 s_sisdb_client_ask

}s_sisdb_client_cxt;

bool  sisdb_client_init(void *, void *);
void  sisdb_client_uninit(void *);
void  sisdb_client_method_init(void *);
void  sisdb_client_method_uninit(void *);

s_sisdb_client_ask *sisdb_client_ask_create(
    const char   *cmd_,            // 请求的参数
	const char   *key_,            // 请求的key
	const char   *val_,            // 请求的参数
	void         *cb_source_,          // 回调传送对象
    void         *cb_sub_start,        // 回调开始
	void         *cb_sub_realtime,     // 订阅进入实时状态
	void         *cb_sub_stop,         // 订阅结束 自动取消订阅
	void         *cb_reply             // 回调的数据
);

void sisdb_client_ask_destroy(void *);

// 
s_sisdb_client_ask *sisdb_client_ask_new(s_sisdb_client_cxt *context, 
    const char   *cmd_,            // 请求的参数
	const char   *key_,                // 请求的key
	const char   *val_,                // 请求的参数
	void         *cb_source_,          // 回调传送对象
    void         *cb_sub_start,        // 回调开始
	void         *cb_sub_realtime,     // 订阅进入实时状态
	void         *cb_sub_stop,         // 订阅结束 自动取消订阅
	void         *cb_reply,            // 回调的数据
    bool          issub);
void sisdb_client_ask_del(s_sisdb_client_cxt *, s_sisdb_client_ask *);

s_sisdb_client_ask *sisdb_client_ask_get(
    s_sisdb_client_cxt *, 	
    const char   *source_         // 来源信息
);

void sisdb_client_ask_unsub(
    s_sisdb_client_cxt *, 	
    const char   *cmd_,         // 来源信息
    const char   *key_         // 来源信息
);

bool sisdb_client_ask_sub_exists(
    s_sisdb_client_cxt *context, 	
    const char   *cmd_,         // 来源信息
    const char   *key_         // 来源信息
);

int cmd_sisdb_client_send(void *worker_, void *argv_);

#endif