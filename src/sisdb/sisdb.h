
//******************************************************
// Copyright (C) 2018, Martin <seerlinecoin@gmail.com>
//*******************************************************

#ifndef _SISDB_H
#define _SISDB_H

#include "sis_core.h"
#include "sis_map.h"
#include "sis_malloc.h"
#include "sis_time.h"
#include "sis_list.h"
#include "sis_json.h"
#include "sis_thread.h"

#define SIS_MAXLEN_CODE  9
#define SIS_MAXLEN_NAME  32
#define SIS_MAXLEN_TABLE 32
#define SIS_MAXLEN_KEY   (SIS_MAXLEN_CODE + SIS_MAXLEN_TABLE)

#define SIS_MAXLEN_COMMAND  64
#define SIS_MAXLEN_STRING  128

#define SIS_MARKET_STATUS_NOINIT    0
#define SIS_MARKET_STATUS_INITED    1  // 正常工作状态
#define SIS_MARKET_STATUS_INITING   2  // 开始初始化
#define SIS_MARKET_STATUS_CLOSE     3  // 已收盘,0&3 此状态下可以初始化

#pragma pack(push, 1)

typedef struct s_sis_db {
	s_sis_sds name;            // 数据库名字 sisdb

	s_sis_json_node    *conf;         // 当前数据表的配置信息 保存时用

	s_sis_map_pointer  *dbs;          // 数据表的字典表 s_sisdb_table 数量为数据表数

	bool   special;  // 是否证券专用， 必须有system表的定义，并且不可删除和更改
					 // 一定会定义了开收市时间，并且可以使用PRICE以及VOLUME，AMOUNT字段属性
	s_sis_map_pointer  *sys_exchs;    // 市场的信息 一个市场一个记录 默认记录为 “00”  s_sisdb_sys_exch
	s_sis_struct_list  *sys_infos;    // 股票的信息 实际的info数据 第一条为默认配置，不同才增加 s_sisdb_sys_info
	s_sis_plan_task    *init_task;    // 初始化任务

	s_sis_map_pointer  *collects;     // 数据集合的字典表 s_sisdb_collect 这里实际存放数据，数量为股票个数x数据表数

	s_sis_map_pointer  *map;          // 关键字查询表
	
	s_sis_map_pointer  *calls;          // 远程过程调用

	bool   loading;  // 数据加载中

	int 				save_format;   // 存盘文件的方式
	s_sis_plan_task    *save_task;

}s_sis_db;

#pragma pack(pop)


s_sis_db *sisdb_create(char *);  //数据库的名称，为空建立一个sys的数据库名
void sisdb_destroy(s_sis_db *);  //关闭一个数据库

int sis_from_node_get_format(s_sis_db *db_, s_sis_json_node *node_);


#endif  /* _SISDB_H */
