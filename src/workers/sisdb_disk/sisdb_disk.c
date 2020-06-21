﻿#include "sis_modules.h"
#include "worker.h"
#include "server.h"
#include "sis_method.h"

#include "sisdb_disk.h"
#include "sisdb.h"
#include "sisdb_collect.h"
///////////////////////////////////////////////////
// *** s_sis_modules sis_modules_[dir name]  *** //
///////////////////////////////////////////////////

struct s_sis_method sisdb_disk_methods[] = {
    {"save", cmd_sisdb_disk_save, NULL, NULL},  // json 格式
    {"pack", cmd_sisdb_disk_pack, NULL, NULL},  // json 格式
};
// 通用文件存取接口
s_sis_modules sis_modules_sisdb_disk = {
    sisdb_disk_init,
    NULL,
    NULL,
    NULL,
    sisdb_disk_uninit,
    sisdb_disk_method_init,
    sisdb_disk_method_uninit,
    sizeof(sisdb_disk_methods) / sizeof(s_sis_method),
    sisdb_disk_methods,
};

bool sisdb_disk_init(void *worker_, void *argv_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sis_json_node *node = (s_sis_json_node *)argv_;

    s_sisdb_disk_cxt *context = SIS_MALLOC(s_sisdb_disk_cxt, context);
    worker->context = context;
    {
        s_sis_json_node *sonnode = sis_json_cmp_child_node(node, "work-path");
        if (sonnode)
        {
            context->work_path = sis_sdsnew(sonnode->value);
        }
        else
        {
            context->work_path = sis_sdsnew("data/");
        }
        if (!sis_path_exists(context->work_path))
        {
            sis_path_mkdir(context->work_path);
        }
    }
    {
        s_sis_json_node *sonnode = sis_json_cmp_child_node(node, "safe-path");
        if (sonnode)
        {
            context->safe_path = sis_sdsnew(sonnode->value);
        }
        else
        {
            context->safe_path = sis_sdsnew("data/safe/");
        }
        if (!sis_path_exists(context->safe_path))
        {
            sis_path_mkdir(context->safe_path);
        }
    }
    context->page_size = sis_json_get_int(node, "page-size", 500) * 1000000;

    return true;
}
void sisdb_disk_uninit(void *worker_)
{
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_disk_cxt *context = (s_sisdb_disk_cxt *)worker->context;
    sis_sdsfree(context->work_path);
    sis_sdsfree(context->safe_path);
    sis_free(context);

}
void sisdb_disk_method_init(void *worker_)
{
}
void sisdb_disk_method_uninit(void *worker_)
{
}

s_sis_sds _sisdb_get_keys(s_sisdb_cxt *cxt)
{
    int nums = 0;
    int count = sis_map_list_getsize(cxt->keys);
    s_sis_sds msg = sis_sdsempty();
    msg = sis_sdscat(msg, "{");
    for(int i = 0; i < count; i++)
    {
        s_sis_sds key = (s_sis_sds)sis_map_list_geti(cxt->keys, i);
        if (nums > 0)
        {
            msg = sis_sdscat(msg, ",");
        }
        nums++;
        msg = sis_sdscatfmt(msg, "\"%S\"", key);
    }
    msg = sis_sdscat(msg, "}");
    printf("keys = %s\n", msg);
    return msg;
}

s_sis_sds _sisdb_get_sdbs(s_sisdb_cxt *cxt, int style)
{
    int nums = 0;
    int count = sis_map_list_getsize(cxt->sdbs);
    s_sis_sds msg = sis_sdsempty();
    {
        msg = sis_sdscat(msg, "{");
        for(int i = 0; i < count; i++)
        {
            s_sisdb_table *sdb = (s_sisdb_table *)sis_map_list_geti(cxt->sdbs, i);
            if (sdb->style != style)
            {
                continue;
            }
            if (nums > 0)
            {
                msg = sis_sdscat(msg, ",");
            }
            nums++;
            msg = sis_sdscatfmt(msg, "\"%S\":", sdb->db->name);
            msg = sis_dynamic_dbinfo_to_json_sds(sdb->db, msg);
        }
        msg = sis_sdscat(msg, "}");
    }
    printf("sdbs = %s\n", msg);
    return msg;
}

int cmd_sisdb_disk_save(void *worker_, void *argv_)
{
    s_sis_message *msg = (s_sis_message *)argv_;
    s_sisdb_cxt *sisdb = (s_sisdb_cxt *)sis_message_get(msg, "sisdb");
    if (!sisdb)
    {
        return SIS_METHOD_ERROR;
    }
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_disk_cxt *context = (s_sisdb_disk_cxt *)worker->context;

    // 先写sno数据
    s_sis_disk_class *snofile = sis_disk_class_create();
    sis_disk_class_init(snofile, SIS_DISK_TYPE_SNO, context->work_path, sisdb->name);
    {
        s_sis_sds keys = _sisdb_get_keys(sisdb);
        sis_disk_class_set_key(snofile, keys, sis_sdslen(keys));
        s_sis_sds sdbs = _sisdb_get_sdbs(sisdb, SISDB_TB_STYLE_SNO);
        sis_disk_class_set_sdb(snofile, sdbs, sis_sdslen(sdbs));
        sis_sdsfree(keys);
        sis_sdsfree(sdbs);
    }
    sis_disk_file_write_start(snofile, SIS_DISK_ACCESS_CREATE);
    
    int count = sis_node_list_get_size(sisdb->series);
    for (int i = 0; i < count; i++)
    {
        s_sisdb_collect_sno *sno = (s_sisdb_collect_sno *)sis_node_list_get(sisdb->series, i);        
        sis_disk_file_write_sdb(snofile, sno->collect->key, sno->collect->sdb->db->name, 
            sis_struct_list_get(sno->collect->value, sno->recno) , sno->collect->sdb->db->size);
    }
    
    sis_disk_file_write_stop(snofile);
    sis_disk_class_destroy(snofile);

    // 再写sdb数据
    s_sis_disk_class *sdbfile = sis_disk_class_create();
    sis_disk_class_init(sdbfile, SIS_DISK_TYPE_SNO, context->work_path, sisdb->name);
    {
        s_sis_sds keys = _sisdb_get_keys(sisdb);
        sis_disk_class_set_key(snofile, keys, sis_sdslen(keys));
        s_sis_sds sdbs = _sisdb_get_sdbs(sisdb, SISDB_TB_STYLE_SDB);
        sis_disk_class_set_sdb(snofile, sdbs, sis_sdslen(sdbs));
        sis_sdsfree(keys);
        sis_sdsfree(sdbs);
    }
    sis_disk_file_write_start(sdbfile, SIS_DISK_ACCESS_APPEND);
    // 写入自由键值
    {
        s_sis_memory *memory = sis_memory_create();
        s_sis_dict_entry *de;
        s_sis_dict_iter *di = sis_dict_get_iter(sisdb->kvs);
        while ((de = sis_dict_next(di)) != NULL)
        {
            s_sisdb_kv *val = (s_sisdb_kv *)sis_dict_getval(de);
            sis_memory_clear(memory);
            sis_memory_cat_byte(memory, val->format, 2);
            sis_memory_cat(memory, val->value, sis_sdslen(val->value));
            sis_disk_file_write_any(sdbfile, sis_dict_getkey(de), sis_memory(memory), sis_memory_get_size(memory));
        }
        sis_dict_iter_free(di);
        sis_memory_destroy(memory);
    }
    // 写入结构键值
    {
        s_sis_dict_entry *de;
        s_sis_dict_iter *di = sis_dict_get_iter(sisdb->collects);
        while ((de = sis_dict_next(di)) != NULL)
        {
            s_sisdb_collect *collect = (s_sisdb_collect *)sis_dict_getval(de);
            if (collect->sdb->style == SISDB_TB_STYLE_SDB)
            {
                sis_disk_file_write_sdb(sdbfile, collect->key, collect->sdb->db->name, 
                    sis_struct_list_first(collect->value), collect->value->count * collect->value->len);
            }
        }
        sis_dict_iter_free(di);
    }
    sis_disk_file_write_stop(sdbfile);
    sis_disk_class_destroy(sdbfile);

    return SIS_METHOD_OK;
}

int cmd_sisdb_disk_pack(void *worker_, void *argv_)
{
    s_sis_message *msg = (s_sis_message *)argv_;
    s_sisdb_cxt *sisdb = (s_sisdb_cxt *)sis_message_get(msg, "sisdb");
    if (!sisdb)
    {
        return SIS_METHOD_ERROR;
    }
    s_sis_worker *worker = (s_sis_worker *)worker_; 
    s_sisdb_disk_cxt *context = (s_sisdb_disk_cxt *)worker->context;
    // 只处理 sdb 的数据 sno 数据本来就是没有冗余的
    s_sis_disk_class *srcfile = sis_disk_class_create();
    sis_disk_class_init(srcfile, SIS_DISK_TYPE_SDB, context->work_path, sisdb->name);
    sis_disk_file_move(srcfile, context->safe_path);
    sis_disk_class_init(srcfile, SIS_DISK_TYPE_SDB, context->safe_path, sisdb->name);

    s_sis_disk_class *desfile = sis_disk_class_create();
    sis_disk_class_init(desfile, SIS_DISK_TYPE_SDB, context->work_path, sisdb->name);

    size_t size = sis_disk_file_pack(srcfile, desfile);

    if (size == 0)
    {
        sis_disk_file_delete(desfile);
        sis_disk_file_move(srcfile, context->work_path);
    }   
    sis_disk_class_destroy(desfile);
    sis_disk_class_destroy(srcfile);

    if (size == 0)
    {
        return SIS_METHOD_ERROR;
    }
    return SIS_METHOD_OK;
}
///////////////////////////
//  s_sis_disk_index
///////////////////////////
s_sis_disk_index *sis_disk_index_create(s_sis_object *key_, s_sis_object *sdb_)
{
    s_sis_disk_index *o = SIS_MALLOC(s_sis_disk_index, o);
    if (key_)
    {
        o->key = sis_object_incr(key_);
    }
    if (sdb_)
    {
        o->sdb = sis_object_incr(sdb_);
    }
    o->index = sis_struct_list_create(sizeof(s_sis_disk_index_unit));
    return o;
}
void sis_disk_index_destroy(void *in_)
{
    s_sis_disk_index *o = (s_sis_disk_index *)in_;
    sis_object_decr(o->key);
    sis_object_decr(o->sdb);
    sis_struct_list_destroy(o->index);
    sis_free(o);
}
int sis_disk_index_set_unit(s_sis_disk_index *cls_, s_sis_disk_index_unit *unit_)
{
    return sis_struct_list_push(cls_->index, unit_);
}

s_sis_disk_index_unit *sis_disk_index_get_unit(s_sis_disk_index *cls_, int index_)
{
    return (s_sis_disk_index_unit *)sis_struct_list_get(cls_->index, index_);
}

////////////////////////////////////////////////////////
// s_sis_disk_dict
////////////////////////////////////////////////////////

s_sis_disk_dict *sis_disk_dict_create(s_sis_json_node *node_, s_sis_dynamic_db * db_)
{
    s_sis_disk_dict *o = SIS_MALLOC(s_sis_disk_dict, o);
    o->name = sis_object_create(SIS_OBJECT_SDS, sis_sdsnew(node_->key));
    o->units = sis_pointer_list_create();
    sis_disk_dict_set(o, node_, db_);
    return o;
}
void sis_disk_dict_destroy(void *in_)
{
    s_sis_disk_dict *info = (s_sis_disk_dict *)in_;
    sis_disk_dict_clear_units(info);
    sis_pointer_list_destroy(info->units);
    sis_object_destroy(info->name);
    sis_free(info);
}

void sis_disk_dict_clear_units(s_sis_disk_dict *dict_)
{
    for (int i = 0; i < dict_->units->count; i++)
    {
        s_sis_disk_dict_unit *unit = (s_sis_disk_dict_unit *)sis_pointer_list_get(dict_->units, i);
        if (unit->db)
        {
            sis_dynamic_db_destroy(unit->db);
        }
        sis_sdsfree(unit->attr);
        sis_free(unit);
    }
    sis_pointer_list_clear(dict_->units);
}
s_sis_disk_dict_unit *sis_disk_dict_last(s_sis_disk_dict *dict_)
{
    return (s_sis_disk_dict_unit *)sis_pointer_list_get(dict_->units, dict_->units->count - 1);
}

s_sis_disk_dict_unit *sis_disk_dict_get(s_sis_disk_dict *dict_, int index_)
{
    return (s_sis_disk_dict_unit *)sis_pointer_list_get(dict_->units, index_);
}

int sis_disk_dict_set(s_sis_disk_dict *dict_, s_sis_json_node *node_, s_sis_dynamic_db * db_)
{
    bool isnew = true;
    s_sis_sds attr = NULL;
    if (node_->type == SIS_JSON_NULL)
    {
        s_sis_disk_dict_unit *last = (s_sis_disk_dict_unit *)sis_pointer_list_get(dict_->units, dict_->units->count - 1);
        if (last && !last->attr)
        {
            isnew = false;
        }
    }
    else
    {
        size_t size;
        char *str = sis_json_output_zip(node_, &size);
        attr = sis_sdsnewlen(str, size);
        sis_free(str);

        s_sis_disk_dict_unit *last = (s_sis_disk_dict_unit *)sis_pointer_list_get(dict_->units, dict_->units->count - 1);
        if (last && !sis_strncasecmp(attr, last->attr, sis_sdslen(last->attr)))
        {
            isnew = false;
            sis_sdsfree(attr);
        }
    }
    if (isnew)
    {
        s_sis_disk_dict_unit *unit = SIS_MALLOC(s_sis_disk_dict_unit, unit);
        unit->writed = 0;
        unit->attr = attr;
        unit->db = db_;
        sis_pointer_list_push(dict_->units, unit);
        return dict_->units->count;
    }
    else
    {
        if (db_)
        {
            sis_dynamic_db_destroy(db_);
        }
        return -1;
    }
    return 0;
}

////////////////////////////////////////////////////////
// s_sis_disk_rcatch
////////////////////////////////////////////////////////

s_sis_disk_rcatch *sis_disk_rcatch_create(uint64 sno_, s_sis_object *key_, s_sis_object *sdb_)
{
    s_sis_disk_rcatch *o = SIS_MALLOC(s_sis_disk_rcatch, o);
    o->sno = sno_;
    if (key_)
    {
        o->key = sis_object_incr(key_);
    }
    if (sdb_)
    {
        o->sdb = sis_object_incr(sdb_);
    }
    o->output = sis_object_create(SIS_OBJECT_MEMORY, sis_memory_create());
    return o;
}
void sis_disk_rcatch_destroy(void *in_)
{
    s_sis_disk_rcatch *info = (s_sis_disk_rcatch *)in_;
    sis_object_decr(info->key);
    sis_object_decr(info->sdb);
    sis_object_destroy(info->output);
    sis_free(info);
}
////////////////////////////////////////////////////////
// s_sis_disk_wcatch
////////////////////////////////////////////////////////

s_sis_disk_wcatch *sis_disk_wcatch_create(s_sis_object *key_, s_sis_object *sdb_)
{
    s_sis_disk_wcatch *o = SIS_MALLOC(s_sis_disk_wcatch, o);
    if (key_)
    {
        o->key = sis_object_incr(key_);
    }
    if (sdb_)
    {
        o->sdb = sis_object_incr(sdb_);
    }
    o->memory = sis_memory_create();
    return o;
}
void sis_disk_wcatch_destroy(void *in_)
{
    s_sis_disk_wcatch *info = (s_sis_disk_wcatch *)in_;
    sis_object_decr(info->key);
    sis_object_decr(info->sdb);
    sis_memory_destroy(info->memory);
    sis_free(info);
}
void sis_disk_wcatch_clear(s_sis_disk_wcatch *in_)
{
    sis_memory_clear(in_->memory);
    memset(&in_->winfo, 0, sizeof(s_sis_disk_index_unit));
}



////////////////////////////////////////////////////////
// s_sis_disk_class
////////////////////////////////////////////////////////
int sis_disk_class_init(s_sis_disk_class *cls_, int style_, const char *fpath_, const char *fname_)
{

    cls_->style = style_;
    sis_strcpy(cls_->fpath, 255, fpath_);
    sis_strcpy(cls_->fname, 255, fname_);

    cls_->work_fps->main_head.style = style_;
    cls_->work_fps->main_head.index = 0;
    cls_->work_fps->main_head.wtime = sis_time_get_idate(0);

    char work_fn[SIS_DISK_NAME_LEN];
    char index_fn[SIS_DISK_NAME_LEN];
    switch (style_)
    {
    case SIS_DISK_TYPE_LOG:  // fname_ 必须为日期
        if (sis_time_str_is_date((char *)fname_))
        {
            sis_strcpy(cls_->midpath, 255, SIS_DISK_LOG_DIR);
            sis_sprintf(work_fn, SIS_DISK_NAME_LEN, "%s/%s/%s.%s",
                                        fpath_, SIS_DISK_LOG_DIR, fname_, SIS_DISK_LOG_CHAR);
            cls_->work_fps->main_head.wtime = sis_time_get_idate_from_str(fname_, 0);
            cls_->work_fps->max_page_size = SIS_DISK_MAXLEN_MIDPAGE;
            cls_->work_fps->max_file_size = 0; // 顺序读写的文件 不需要设置最大值 
        }
        else
        {
            LOG(8)("get file name no date [%s].\n", fname_);
            return -1;
        }
        break;
    case SIS_DISK_TYPE_SNO: // fname_ 必须为日期
        if (sis_time_str_is_date((char *)fname_))
        {
            sis_strcpy(cls_->midpath, 255, SIS_DISK_SNO_DIR);
            sis_sprintf(work_fn, SIS_DISK_NAME_LEN, "%s/%s/%s.%s",
                                      fpath_, SIS_DISK_SNO_DIR, fname_, SIS_DISK_SNO_CHAR);
            cls_->work_fps->main_head.index = 1;
            cls_->work_fps->main_head.wtime = sis_time_get_idate_from_str(fname_, 0);
            cls_->work_fps->max_page_size = SIS_DISK_MAXLEN_MAXPAGE;
            // cls_->work_fps->max_page_size = SIS_DISK_MAXLEN_MIDPAGE;
            cls_->work_fps->max_file_size = SIS_DISK_MAXLEN_FILE;
            sis_sprintf(index_fn, SIS_DISK_NAME_LEN, "%s/%s/%s.%s",
                                       fpath_, SIS_DISK_SNO_DIR, fname_, SIS_DISK_IDX_CHAR);
        }
        else
        {
            LOG(8)("get file name no date [%s].\n", fname_);
            return -2;
        }
        break;
    case SIS_DISK_TYPE_SDB:
        sis_sprintf(work_fn, SIS_DISK_NAME_LEN, "%s/%s.%s",
                                      fpath_, fname_, SIS_DISK_SDB_CHAR);
        cls_->work_fps->main_head.index = 1;
        cls_->work_fps->max_page_size = 0;
        cls_->work_fps->max_file_size = SIS_DISK_MAXLEN_FILE;
        sis_sprintf(index_fn, SIS_DISK_NAME_LEN, "%s/%s.%s",
                                       fpath_, fname_, SIS_DISK_IDX_CHAR);
        break;
    default: // SIS_DISK_TYPE_STREAM
        // 其他类型的文件直接传入文件名 用户可自定义类型
        sis_sprintf(work_fn, SIS_DISK_NAME_LEN, "%s/%s", fpath_, fname_);
        cls_->work_fps->max_page_size = SIS_DISK_MAXLEN_MIDPAGE;
        cls_->work_fps->max_file_size = 0; // 顺序读写的文件 不需要设置最大值 
        break;
    }
    sis_files_init(cls_->work_fps, work_fn);

    char crc[32]; 
    sis_str_get_time_id(crc, 15);
    // 工作文件和索引文件保持同样的随机码
    memmove(cls_->work_fps->main_tail.crc, crc, 15);

    if (cls_->work_fps->main_head.index)
    {
        cls_->index_fps->main_head.hid = SIS_DISK_HID_HEAD;
        cls_->index_fps->main_head.style = style_ == SIS_DISK_TYPE_SNO ? SIS_DISK_TYPE_SNO_IDX : SIS_DISK_TYPE_SDB_IDX;
        cls_->index_fps->max_file_size = SIS_DISK_MAXLEN_INDEX;
        cls_->index_fps->max_page_size = SIS_DISK_MAXLEN_MINPAGE;

        cls_->index_fps->main_head.wtime = sis_time_get_idate(0);

        sis_files_init(cls_->index_fps, index_fn);

        memmove(cls_->index_fps->main_tail.crc, crc, 15);
    }
    cls_->isinit = true;
    return 0;
}

// s_sis_disk_class *sis_disk_class_create(int style_, const char *fpath_, const char *fname_)
s_sis_disk_class *sis_disk_class_create()
{
    s_sis_disk_class *cls = SIS_MALLOC(s_sis_disk_class, cls);

    sis_mutex_init(&cls->mutex, NULL);

    cls->keys = sis_map_list_create(sis_disk_dict_destroy);
    cls->sdbs = sis_map_list_create(sis_disk_dict_destroy);

    cls->src_wcatch = sis_disk_wcatch_create(NULL, NULL);

    cls->index_infos = sis_map_list_create(sis_disk_index_destroy);
    
    cls->sno_wcatch = sis_map_list_create(sis_disk_wcatch_destroy);
    cls->sno_rcatch = sis_pointer_list_create();
    cls->sno_rcatch->vfree = sis_disk_rcatch_destroy;

    cls->work_fps = sis_files_create();
    cls->index_fps = sis_files_create();

    cls->status = SIS_DISK_STATUS_CLOSED;
    // if (sis_disk_class_init(cls, style_, fpath_, fname_))
    // {
    //     sis_disk_class_destroy(cls);
    //     return NULL;
    // }

    return cls;
}

void sis_disk_class_destroy(s_sis_disk_class *cls_)
{
    sis_mutex_destroy(&cls_->mutex);

    sis_map_list_destroy(cls_->keys);
    sis_map_list_destroy(cls_->sdbs);

    sis_disk_wcatch_destroy(cls_->src_wcatch);

    sis_map_list_destroy(cls_->index_infos);

    sis_map_list_destroy(cls_->sno_wcatch);   
    sis_pointer_list_destroy(cls_->sno_rcatch);

    sis_files_destroy(cls_->work_fps);
    sis_files_destroy(cls_->index_fps);

    sis_free(cls_);
}

void sis_disk_class_clear(s_sis_disk_class *cls_)
{
    // 创建时的信息不能删除 其他信息可清理
    sis_mutex_lock(&cls_->mutex);

    sis_files_clear(cls_->work_fps);
    sis_files_clear(cls_->index_fps);

    sis_map_list_clear(cls_->keys);
    sis_map_list_clear(cls_->sdbs);

    sis_disk_wcatch_clear(cls_->src_wcatch);

    sis_map_list_clear(cls_->index_infos);

    cls_->sno_size = 0;
    cls_->sno_series = 0;
    sis_map_list_clear(cls_->sno_wcatch);
  
    sis_pointer_list_clear(cls_->sno_rcatch);

    sis_mutex_unlock(&cls_->mutex);

    sis_disk_class_init(cls_, cls_->style, cls_->fpath, cls_->fname);
}

// 设置文件大小
void sis_disk_class_set_size(s_sis_disk_class *cls_,size_t fsize_, size_t psize_)
{
    cls_->work_fps->max_file_size = fsize_;
    cls_->work_fps->max_page_size = psize_;
    if (cls_->style == SIS_DISK_TYPE_STREAM || cls_->style == SIS_DISK_TYPE_LOG)
    {
        cls_->work_fps->max_file_size = 0;
    }
}

// 得到key的索引
int sis_disk_class_get_keyi(s_sis_disk_class *cls_, const char *key_)
{
    return sis_map_list_get_index(cls_->keys, key_);
}
// 得到sdb的索引
int sis_disk_class_get_sdbi(s_sis_disk_class *cls_, const char *sdb_)
{
    return sis_map_list_get_index(cls_->sdbs, sdb_);
}
const char *sis_disk_class_get_keyn(s_sis_disk_class *cls_, int keyi_)
{
    return SIS_OBJ_SDS(((s_sis_disk_dict *)sis_map_list_geti(cls_->keys, keyi_))->name);
}
const char *sis_disk_class_get_sdbn(s_sis_disk_class *cls_, int sdbi_)
{
    return SIS_OBJ_SDS(((s_sis_disk_dict *)sis_map_list_geti(cls_->sdbs, sdbi_))->name);
}

int sis_disk_class_set_key(s_sis_disk_class *cls_, const char *in_, size_t ilen_)
{
    s_sis_json_handle *injson = sis_json_load(in_, ilen_);
    if (!injson)
    {
        sis_json_close(injson);
        return 0;
    }
    s_sis_json_node *innode = sis_json_first_node(injson->node); 
    while (innode)
    {   
        // printf("%s\n",innode->key ? innode->key : "null");
        s_sis_disk_dict *info = sis_map_list_get(cls_->keys, innode->key);
        if (!info)
        {
            info = sis_disk_dict_create(innode, NULL);
            info->index = sis_map_list_set(cls_->keys, innode->key, info);
        }
        else
        {
            sis_disk_dict_set(info, innode, NULL);
        }
        innode = sis_json_next_node(innode);
    }
    sis_json_close(injson);
    return  sis_map_list_getsize(cls_->keys);    
}
// 设置结构体
int sis_disk_class_set_sdb(s_sis_disk_class *cls_, const char *in_, size_t ilen_)
{
    s_sis_json_handle *injson = sis_json_load(in_, ilen_);
    if (!injson)
    {
        sis_json_close(injson);
        return 0;
    }
   s_sis_json_node *innode = sis_json_first_node(injson->node); 
    while (innode)
    {
        s_sis_dynamic_db *sdb = sis_dynamic_db_create(innode);
        if (sdb)
        {
            s_sis_disk_dict *info = sis_map_list_get(cls_->sdbs, innode->key);
            if (!info)
            {
                info = sis_disk_dict_create(innode, sdb);
                info->index = sis_map_list_set(cls_->sdbs, innode->key, info);
            }
            else
            {
                sis_disk_dict_set(info, innode, sdb);
            }
        }  
        innode = sis_json_next_node(innode);
    }
    sis_json_close(injson);
    // for (int i = 0; i < cls_->sdbs->list->count; i++)
    // {
    //     s_sis_disk_dict *info = sis_map_list_geti(cls_->sdbs, i);
    //     printf("set sdb : %s, %d\n", SIS_OBJ_SDS(info->name), info->index);
    // }
    
    return  sis_map_list_getsize(cls_->sdbs);    
}

size_t sis_disk_file_pack(s_sis_disk_class *src_, s_sis_disk_class *des_)
{
    // sis_disk_file_read_start(srcfile);
    // sis_disk_file_read_stop(srcfile);

    return 0;
}

#if 0

int __nums = 0;
size_t __size = 0;

void cb_begin(void *src, msec_t tt)
{
    printf("%s : %llu\n", __func__, tt);
}
void cb_key(void *src, void *key_, size_t size) 
{
    __size += size;
    s_sis_sds info = sis_sdsnewlen((char *)key_, size);
    printf("%s %d :%s\n", __func__, (int)size, info);
    sis_sdsfree(info);
}
void cb_sdb(void *src, void *sdb_, size_t size)  
{
    __size += size;
    s_sis_sds info = sis_sdsnewlen((char *)sdb_, size);
    printf("%s %d : %s\n", __func__, (int)size, info);
    sis_sdsfree(info);
}

void cb_read_stream(void *src, const char *key_, const char *sdb_, s_sis_object *obj_)
{
    __nums++;
    __size += sis_memory_get_size((s_sis_memory *)obj_->ptr);
    // if (__nums < 2 || __nums % 1000 == 0)
    {
        printf("%s : %d %zu\n", __func__, __nums, __size);
        sis_out_binary(" ", sis_memory((s_sis_memory *)obj_->ptr), 16);
                // sis_memory_get_size((s_sis_memory *)obj_->ptr));
    }
} 

void cb_read(void *src, const char *key_, const char *sdb_, s_sis_object *obj_)
{
    __nums++;
    __size += sis_memory_get_size((s_sis_memory *)obj_->ptr);
    // if (__nums < 2 || __nums % 100 == 0)
    {
        printf(" %s : [%d] %zu ",__func__, __nums, __size);
        if (key_) printf(" %s ", key_);
        if (sdb_) printf(" %s ", sdb_);
        
        s_sis_disk_class *rwf = (s_sis_disk_class *)src;
        s_sis_disk_dict *sdb = (s_sis_disk_dict *)sis_map_list_get(rwf->sdbs, sdb_);
        if (sdb)
        {
            s_sis_disk_dict_unit *unit =  sis_disk_dict_last(sdb);
            s_sis_sds info = sis_dynamic_db_to_csv_sds(unit->db, sis_memory((s_sis_memory *)obj_->ptr), 
                    sis_memory_get_size((s_sis_memory *)obj_->ptr)); 
            printf(" %s \n", info);
            sis_sdsfree(info);
        }
        else
        {
            sis_out_binary("data:", sis_memory((s_sis_memory *)obj_->ptr), 16);
        }
        
    }
} 
void cb_end(void *src, msec_t tt)
{
    printf("%s : %llu\n", __func__, tt);
}
// test stream
int test_stream()
{
    // s_sis_sds s = sis_sdsempty();
    // char c = 'A';
    // s = sis_sdscatfmt(s,"%s", &c);
    // printf("%c %s\n", c, s);
    // return 0;
    sis_log_open(NULL, 10, 0);
    safe_memory_start();
    s_sis_disk_class *rwf = sis_disk_class_create();
    // 先写
    sis_disk_class_init(rwf, SIS_DISK_TYPE_STREAM, "dbs", "1111.aof");

    sis_disk_file_write_start(rwf, SIS_DISK_ACCESS_CREATE);

    int count = 1*1000*1000;
    sis_disk_class_set_size(rwf, 400*1000, 20*1000);
    // sis_disk_class_set_size(rwf, 0, 300*1000);
    for (int i = 0; i < count; i++)
    {
        sis_disk_file_write_stream(rwf, "012345678987654321", 18);
    }  
    sis_disk_file_write_stop(rwf);
    // 后读
    printf("write end .\n");

    sis_disk_file_read_start(rwf);
    s_sis_disk_callback *callback = SIS_MALLOC(s_sis_disk_callback, callback);
    callback->source = rwf;
    callback->cb_begin = cb_begin;
    callback->cb_read = cb_read_stream;
    callback->cb_end = cb_end;
    __nums = 0;

    s_sis_disk_reader *reader = sis_disk_reader_create(callback);
    sis_disk_file_read_sub(rwf, reader);

    sis_disk_file_read_stop(rwf);
    sis_disk_reader_destroy(reader);
    sis_free(callback);
    printf("read end __nums = %d \n", __nums);

    sis_disk_class_destroy(rwf);
    safe_memory_stop();
    sis_log_close();
    return 0;
}


// const char *keys = "{\"k1\",\"k3\",\"k3\"}";
char *keys = "{\"k1\":{\"dp\":2},\"k2\":{\"dp\":2},\"k3\":{\"dp\":3}}";
char *sdbs = "{\"info\":{\"fields\":{\"name\":[\"C\",10]}},\"snap\":{\"fields\":{\"time\":[\"U\",8],\"newp\":[\"F\",8,1,2],\"vol\":[\"I\",4]}}}";
#pragma pack(push,1)
typedef struct s_info
{
    char name[10];
}s_info;
typedef struct s_snap
{
    uint64 time;
    double newp; 
    uint32 vol;
}s_snap;
#pragma pack(pop)
s_info info_data[3] = { 
    { "k10000" },
    { "k20000" },
    { "k30000" },
};
s_snap snap_data[10] = { 
    { 1000, 128.561, 3000},
    { 2000, 129.562, 4000},
    { 3000, 130.563, 5000},
    { 4000, 121.567, 6000},
    { 5000, 122.561,  7000},
    { 6000, 123.562,  8000},
    { 7000, 124.563,  9000},
    { 8000, 125.564,  9900},
    { 9000, 124.563,  9000},
    { 10000, 125.564,  9900},
};
size_t write_hq_1(s_sis_disk_class *rwf)
{
    size_t size = 0;
    int count  = 1000;
    for (int k = 0; k < count; k++)
    {
        for (int i = 0; i < 8; i++)
        {
            size += sis_disk_file_write_sdb(rwf, "k1", "snap", &snap_data[i], sizeof(s_snap));
        }
        // printf("%zu\n", size);
        for (int i = 0; i < 5; i++)
        {
            size += sis_disk_file_write_sdb(rwf, "k2", "snap", &snap_data[i], sizeof(s_snap));
        }
        // printf("%zu\n", size);
        for (int i = 0; i < 7; i++)
        {
            size += sis_disk_file_write_sdb(rwf, "k3", "snap", &snap_data[i], sizeof(s_snap));
        }
    }
    return size;
}

size_t write_hq(s_sis_disk_class *rwf)
{
    size_t size = 0;
    int count  = 3000;
    s_snap snap;
    int sno = 1;
    for (int k = 0; k < count; k++)
    {        
        for (int i = 0; i < 10; i++)
        {
            snap.time = sno++;
            snap.newp = 100.12 + i;
            snap.vol  = 10000*k + i;
            size += sis_disk_file_write_sdb(rwf, "k1", "snap", &snap, sizeof(s_snap));
        }
        // printf("%zu\n", size);
        for (int i = 0; i < 10; i++)
        {
            snap.time = sno++;
            snap.newp = 200.12 + i;
            snap.vol  = 20000*k + i;
            size += sis_disk_file_write_sdb(rwf, "k2", "snap", &snap, sizeof(s_snap));
        }
        // printf("%zu\n", size);
        for (int i = 0; i < 10; i++)
        {
            snap.time = sno++;
            snap.newp = 400.12 + i;
            snap.vol  = 40000*k + i;
            size += sis_disk_file_write_sdb(rwf, "k3", "snap", &snap, sizeof(s_snap));
        }
    }
    return size;
}
size_t write_after(s_sis_disk_class *rwf)
{
    size_t size = 0;
    int count  = 500;
    int sno = 1;
    for (int k = 0; k < count; k++)
    {        
        for (int i = 0; i < 10; i++)
        {
            snap_data[i].time = sno++;
            snap_data[i].newp = 100.12 + i;
            snap_data[i].vol  = 10000*k + i;
        }
        size += sis_disk_file_write_sdb(rwf, "k1", "snap", &snap_data[0], 10 * sizeof(s_snap));
        // printf("%zu\n", size);
        for (int i = 0; i < 10; i++)
        {
            snap_data[i].time = sno++;
            snap_data[i].newp = 200.12 + i;
            snap_data[i].vol  = 20000*k + i;
        }
        size += sis_disk_file_write_sdb(rwf, "k2", "snap", &snap_data[0], 10 * sizeof(s_snap));
        // printf("%zu\n", size);
        for (int i = 0; i < 10; i++)
        {
            snap_data[i].time = sno++;
            snap_data[i].newp = 400.12 + i;
            snap_data[i].vol  = 40000*k + i;
        }
        size += sis_disk_file_write_sdb(rwf, "k3", "snap", &snap_data[0], 10 * sizeof(s_snap));
    }
    return size;
}
void write_log(s_sis_disk_class *rwf)
{
    // 先写
    sis_disk_class_set_key(rwf, keys, sis_strlen(keys));
    sis_disk_class_set_sdb(rwf, sdbs, sis_strlen(sdbs));

    sis_disk_file_write_start(rwf, SIS_DISK_ACCESS_CREATE);
    

    // int count = 1*1000*1000;
    sis_disk_class_set_size(rwf, 400*1000000, 20*1000);
    // sis_disk_class_set_size(rwf, 0, 300*1000);    
    size_t size = sis_disk_file_write_sdb(rwf, "k1", "info", &info_data[0], sizeof(s_info));
    size += sis_disk_file_write_sdb(rwf, "k2", "info", &info_data[1], sizeof(s_info));
    size += sis_disk_file_write_sdb(rwf, "k3", "info", &info_data[2], sizeof(s_info));
    
    size+= write_hq(rwf);

    printf("style : %d, size = %zu\n", rwf->work_fps->main_head.style, size);

    sis_disk_file_write_stop(rwf);
    
    printf("write end\n");
}

void write_sdb(s_sis_disk_class *rwf, char *keys_, char *sdbs_)
{
    // 先写
    if (keys_) sis_disk_class_set_key(rwf, keys_, sis_strlen(keys_));
    if (sdbs_) sis_disk_class_set_sdb(rwf, sdbs_, sis_strlen(sdbs_));

    sis_disk_file_write_start(rwf, SIS_DISK_ACCESS_CREATE);
    // int count = 1*1000*1000;
    sis_disk_class_set_size(rwf, 400*1000, 20*1000);
    // sis_disk_class_set_size(rwf, 0, 300*1000);    
    size_t size = sis_disk_file_write_sdb(rwf, "k1", "info", &info_data[0], sizeof(s_info));
    size += sis_disk_file_write_sdb(rwf, "k2", "info", &info_data[1], sizeof(s_info));
    size += sis_disk_file_write_sdb(rwf, "k3", "info", &info_data[2], sizeof(s_info));
    
    size+= write_after(rwf);

    printf("%zu\n", size);

    sis_disk_file_write_stop(rwf);
    
    printf("write end\n");
}
void read_of_sub(s_sis_disk_class *rwf)
{
    // 后读
    sis_disk_file_read_start(rwf);
    
    s_sis_disk_callback *callback = SIS_MALLOC(s_sis_disk_callback, callback);
    callback->source = rwf;
    callback->cb_begin = cb_begin;
    callback->cb_key = cb_key;
    callback->cb_sdb = cb_sdb;
    callback->cb_read = cb_read;
    callback->cb_end = cb_end;

    s_sis_disk_reader *reader = sis_disk_reader_create(callback);
    sis_disk_reader_set_key(reader, "k1");
    // sis_disk_reader_set_sdb(reader, "snap");
    // sis_disk_reader_set_sdb(reader, "*");
    sis_disk_reader_set_sdb(reader, "info");
    sis_disk_reader_set_time(reader, 1000, 1500);

    __nums = 0;
    // sub 是一条一条的输出
    // sis_disk_file_read_sub(rwf, reader);
    // get 是所有符合条件的一次性输出
    sis_disk_file_read_get(rwf, reader);

    sis_disk_reader_destroy(reader);
    sis_free(callback);

    sis_disk_file_read_stop(rwf);
    
    printf("read end __nums = %d \n", __nums);

}

int main()
{
    sis_log_open(NULL, 10, 0);
    safe_memory_start();

// test stream
    // test_stream();
// test log
    // s_sis_disk_class *rwf = sis_disk_class_create(SIS_DISK_TYPE_LOG ,"dbs", "20200101");  
    // write_log(rwf);
    // read_of_sub(rwf);
    // sis_disk_class_destroy(rwf);

// test sno
    s_sis_disk_class *rwf = sis_disk_class_create();//SIS_DISK_TYPE_SNO ,"dbs", "20200101");  
    sis_disk_class_init(rwf, SIS_DISK_TYPE_SNO, "dbs", "20200101");
    write_log(rwf);
    read_of_sub(rwf);
    sis_disk_class_destroy(rwf);

// test other
    // s_sis_disk_class *rwf = sis_disk_class_create(SIS_DISK_TYPE_SDB ,"dbs", "20200101");  
    // // write_sdb(rwf, keys, sdbs);  // sdb
    // write_sdb(rwf, NULL, sdbs);  // kdb
    // // write_sdb(rwf, keys, NULL);  // key
    // // write_sdb(rwf, NULL, NULL);  // any
    // read_of_sub(rwf);
    // sis_disk_class_destroy(rwf);
    
    safe_memory_stop();
    sis_log_close();
    return 0;
}
#endif
