﻿

#include <sisdb_collect.h>
#include <sisdb_io.h>
#include <sisdb.h>

///////////////////////////////////////////////////////////////////////////
//------------------------s_sis_step_index --------------------------------//
///////////////////////////////////////////////////////////////////////////

s_sis_step_index *sisdb_stepindex_create()
{
	s_sis_step_index *si = SIS_MALLOC(s_sis_step_index, si);
	return si;
}
void sisdb_stepindex_destroy(s_sis_step_index *si_)
{
	sis_free(si_);
}
void sisdb_stepindex_clear(s_sis_step_index *si_)
{
	si_->left = 0;
	si_->right = 0;
	si_->count = 0;
	si_->step = 0.0;
}
void sisdb_stepindex_rebuild(s_sis_step_index *si_, uint64 left_, uint64 right_, int count_)
{
	si_->left = left_;
	si_->right = right_;
	si_->count = count_;
	si_->step = 0.0;
	if (count_ > 0)
	{
		si_->step = (right_ - left_) / count_;
	}
}
int sisdb_stepindex_goto(s_sis_step_index *si_, uint64 curr_)
{
	if (si_->count < 1)
	{
		return -1;
	}
	if (curr_ <= si_->left)
	{
		return 0;
	}
	if (curr_ >= si_->right)
	{
		return si_->count - 1;
	}
	// printf("goto %f\n", si_->step);
	int index = 0;
	if (si_->step > 1.000001)
	{
		index = (int)((curr_ - si_->left) / si_->step);
	}
	if (index > si_->count - 1)
	{
		return si_->count - 1;
	}
	return index;
}
///////////////////////////////////////////////////////////////////////////
//------------------------s_sisdb_collect --------------------------------//
///////////////////////////////////////////////////////////////////////////

s_sisdb_collect *sisdb_get_collect(s_sisdb_cxt *sisdb_, const char *key_)
{
	s_sisdb_collect *collect = NULL;
	if (sisdb_->collects)
	{
		collect = sis_map_pointer_get(sisdb_->collects, key_);
	}
	return collect;
}
s_sisdb_table *sisdb_get_table(s_sisdb_cxt *sisdb_, const char *dbname_)
{
	s_sisdb_table *tb = (s_sisdb_table *)sis_map_list_get(sisdb_->sdbs, dbname_);
	return tb;
}

s_sisdb_collect *sisdb_collect_create(s_sisdb_cxt *sisdb_, const char *key_)
{
	char code[128];
	char dbname[128];
    sis_str_divide(key_, '.', code, dbname);

	s_sisdb_table *sdb = sisdb_get_table(sisdb_, dbname);
	if (!sdb)
	{
		return NULL;
	}
	s_sisdb_collect *o = SIS_MALLOC(s_sisdb_collect, o);
	sis_map_pointer_set(sisdb_->collects, key_, o);

	o->key = sis_sdsnew(code);
	o->father = sisdb_;
	o->sdb = sdb;

	o->stepinfo = sisdb_stepindex_create();

	o->value = sis_struct_list_create(sdb->db->size);

	return o;
}

void sisdb_collect_destroy(void *collect_)
{
	s_sisdb_collect *collect = (s_sisdb_collect *)collect_;
	{
		sis_struct_list_destroy(collect->value);
	}
	if (collect->stepinfo)
	{
		sisdb_stepindex_destroy(collect->stepinfo);
	}
	sis_sdsfree(collect->key);
	sis_free(collect);
}

void sisdb_collect_clear(s_sisdb_collect *collect_)
{
	sis_struct_list_clear(collect_->value);
	sisdb_stepindex_clear(collect_->stepinfo);
}

////////////////////////
//
////////////////////////
msec_t sisdb_collect_get_time(s_sisdb_collect *collect_, int index_)
{
	return sis_dynamic_db_get_time(collect_->sdb->db, index_, 
		sis_struct_list_first(collect_->value), 
		collect_->value->count * collect_->value->len);
}

uint64 sisdb_collect_get_mindex(s_sisdb_collect *collect_, int index_)
{
	return sis_dynamic_db_get_mindex(collect_->sdb->db, index_, 
		sis_struct_list_first(collect_->value), 
		collect_->value->count * collect_->value->len);
}
int sisdb_collect_recs(s_sisdb_collect *collect_)
{
	if (!collect_ || !collect_->value)
	{
		return 0;
	}
	return collect_->value->count;
}

////////////////////////////////////////////
// format trans
///////////////////////////////////////////
s_sis_json_node *sis_collect_get_fields_of_json(s_sisdb_collect *collect_, s_sis_string_list *fields_)
{
	s_sis_json_node *node = sis_json_create_object();

	// 先处理字段
	int  sno = 0;
	char sonkey[64];
	int count = sis_string_list_getsize(fields_);
	for (int i = 0; i < count; i++)
	{
		const char *key = sis_string_list_get(fields_, i);
		s_sisdb_field *fu = sis_dynamic_db_get_field(collect_->sdb->db, NULL, (char *)key);
		if(!fu) continue;
		if (fu->count > 1)
		{
			for(int index = 0; index < fu->count; index++)
			{
				sis_sprintf(sonkey, 64, "%s.%d", key, index);
				sis_json_object_add_uint(node, sonkey, sno);
				sno++;
			}
		}
		else
		{
			sis_json_object_add_uint(node, key, sno);
			sno++;
		}		
	}
	// sis_json_object_add_node(jone, SIS_JSON_KEY_FIELDS, jfields);
	return node;
}

s_sis_sds sis_collect_get_fields_of_csv(s_sisdb_collect *collect_, s_sis_string_list *fields_)
{
	s_sis_sds o = sis_sdsempty();

	// 先处理字段
	int  sno = 0;
	char sonkey[64];
	int count = sis_string_list_getsize(fields_);
	for (int i = 0; i < count; i++)
	{
		const char *key = sis_string_list_get(fields_, i);
		s_sisdb_field *fu = sis_dynamic_db_get_field(collect_->sdb->db, NULL, (char *)key);
		if(!fu) continue;
		if (fu->count > 1)
		{
			for(int index = 0; index < fu->count; index++)
			{
				sis_sprintf(sonkey, 64, "%s.%d", key, index);
				o = sis_csv_make_str(o, sonkey, sis_strlen(sonkey));
				sno++;
			}
		}
		else
		{
			o = sis_csv_make_str(o, key, sis_strlen(key));
			sno++;
		}		
	}
	o = sis_csv_make_end(o);
	return o;
}

s_sis_sds sisdb_collect_struct_to_sds(s_sisdb_collect *collect_, s_sis_sds in_, s_sis_string_list *fields_)
{
	// 一定不是全字段
	s_sis_sds o = NULL;

	int count = (int)(sis_sdslen(in_) / collect_->value->len);
	char *val = in_;
	for (int k = 0; k < count; k++)
	{
		for (int i = 0; i < sis_string_list_getsize(fields_); i++)
		{
			int index = 0;
			s_sisdb_field *fu = sis_dynamic_db_get_field(collect_->sdb->db, &index, sis_string_list_get(fields_, i));
			if (!fu)
			{
				continue;
			}
			if (!o)
			{
				o = sis_sdsnewlen(val + fu->offset + index * fu->len, fu->len);
			}
			else
			{
				o = sis_sdscatlen(o, val + fu->offset + index * fu->len, fu->len);
			}
		}
		val += collect_->sdb->db->size;
	}
	return o;
}

s_sis_sds sisdb_collect_struct_to_json_sds(s_sisdb_collect *collect_, s_sis_sds in_,
										   const char *key_, s_sis_string_list *fields_,  bool isfields_, bool zip_)
{
	
	int fnums = sis_string_list_getsize(fields_);

	s_sis_json_node *jone = sis_json_create_object();
	
	if (isfields_)
	{
		s_sis_json_node *jfields = sis_json_create_object();
		for (int i = 0; i < fnums; i++)
		{
			s_sis_dynamic_field *inunit = (s_sis_dynamic_field *)sis_map_list_get(collect_->sdb->db->fields, sis_string_list_get(fields_, i));
			sis_json_object_add_uint(jfields, inunit->name, i);
		}
		sis_json_object_add_node(jone, "fields", jfields);
	}

	s_sis_json_node *jtwo = sis_json_create_array();
	const char *val = in_;
	int count = (int)(sis_sdslen(in_) / collect_->sdb->db->size);
	for (int k = 0; k < count; k++)
	{
		s_sis_json_node *jval = sis_json_create_array();
		for (int i = 0; i < fnums; i++)
		{
			s_sis_dynamic_field *inunit = (s_sis_dynamic_field *)sis_map_list_get(collect_->sdb->db->fields, sis_string_list_get(fields_, i));
			sis_dynamic_field_to_array(jval, inunit, val);
		}
		sis_json_array_add_node(jtwo, jval);
		val += collect_->sdb->db->size;
	}
	sis_json_object_add_node(jone, key_? key_ : "values", jtwo);

	s_sis_sds o = sis_json_to_sds(jone, zip_);

	sis_json_delete_node(jone);	
	return o;
}

s_sis_sds sisdb_collect_struct_to_array_sds(s_sisdb_collect *collect_, s_sis_sds in_,
											s_sis_string_list *fields_, bool zip_)
{
	s_sis_json_node *jone = sis_json_create_array();
	int fnums = sis_string_list_getsize(fields_);

	const char *val = in_;
	int count = (int)(sis_sdslen(in_) / collect_->sdb->db->size);
	for (int k = 0; k < count; k++)
	{
		s_sis_json_node *jval = sis_json_create_array();
		for (int i = 0; i < fnums; i++)
		{
			s_sis_dynamic_field *inunit = (s_sis_dynamic_field *)sis_map_list_get(collect_->sdb->db->fields, sis_string_list_get(fields_, i));
			sis_dynamic_field_to_array(jval, inunit, val);
		}
		sis_json_array_add_node(jone, jval);
		val += collect_->sdb->db->size;
	}
	s_sis_sds o = sis_json_to_sds(jone, zip_);
	sis_json_delete_node(jone);	
	return o;
}

s_sis_sds sisdb_collect_struct_to_csv_sds(s_sisdb_collect *collect_, s_sis_sds in_,
										  s_sis_string_list *fields_, bool isfields_)
{
	s_sis_sds o = sis_sdsempty();
	int fnums = sis_string_list_getsize(fields_);
	if (isfields_)
	{
		for (int i = 0; i < fnums; i++)
		{
			s_sis_dynamic_field *inunit = (s_sis_dynamic_field *)sis_map_list_get(collect_->sdb->db->fields, sis_string_list_get(fields_, i));
			o = sis_csv_make_str(o, inunit->name, sis_sdslen(inunit->name));
		}
		o = sis_csv_make_end(o);
	}

	char *val = in_;
	int count = (int)(sis_sdslen(in_) / collect_->value->len);

	for (int k = 0; k < count; k++)
	{
		for (int i = 0; i < fnums; i++)
		{
			s_sis_dynamic_field *inunit = (s_sis_dynamic_field *)sis_map_list_get(collect_->sdb->db->fields, sis_string_list_get(fields_, i));
			o = sis_dynamic_field_to_csv(o, inunit, val);
			val += collect_->value->len;
		}
		o = sis_csv_make_end(o);
		val += collect_->sdb->db->size;
	}
	return o;
}


s_sis_sds sisdb_collect_json_to_struct_sds(s_sisdb_collect *collect_, s_sis_sds in_)
{
	s_sis_json_handle *handle = sis_json_load(in_, sis_sdslen(in_));
	if (!handle)
	{
		return NULL;
	}
	// 取最后一条记录的数据作为底
	const char *src = sis_struct_list_last(collect_->value);
	s_sis_sds o = sis_sdsnewlen(src, collect_->value->len);

	s_sisdb_table *tb = collect_->sdb;

	int index = 0;
	int fnums = sis_string_list_getsize(tb->fields);
	char key[64];
	for (int k = 0; k < fnums; k++)
	{
		s_sisdb_field *fu = (s_sisdb_field *)sis_map_list_geti(tb->db->fields, k);
		for (int i = 0; i < fu->count; i++)
		{
			sis_sprintf(key, 64, "%s.%d", fu->name, i);
			sis_dynamic_field_json_to_struct(o + index * collect_->value->len, fu, i, key, handle->node);
		}
	}
	sis_json_close(handle);
	return o;
}

s_sis_sds sisdb_collect_array_to_struct_sds(s_sisdb_collect *collect_, s_sis_sds in_)
{
	// 字段个数一定要一样
	s_sis_json_handle *handle = sis_json_load(in_, sis_sdslen(in_));
	if (!handle)
	{
		return NULL;
	}
	// 获取字段个数
	s_sis_json_node *jval = NULL;

	int count = 0;
	if (handle->node->child && handle->node->child->type == SIS_JSON_ARRAY)
	{ // 表示二维数组
		jval = handle->node->child;
		count = sis_json_get_size(handle->node);
	}
	else
	{
		count = 1;
		jval = handle->node;
	}
	if (count < 1)
	{
		sis_json_close(handle);
		return NULL;
	}

	int fnums = sis_string_list_getsize(collect_->sdb->fields);
	s_sis_sds o = sis_sdsnewlen(NULL, count * collect_->value->len);
	int index = 0;
	while (jval)
	{
		int size = sis_json_get_size(jval);
		if (size != fnums)
		{
			LOG(3)("input fields[%d] count error [%d].\n", size, fnums);
			jval = jval->next;
			continue;
		}
		char key[32];
		int fidx = 0;
		for (int k = 0; k < fnums; k++)
		{
			s_sisdb_field *fu = (s_sisdb_field *)sis_map_list_geti(collect_->sdb->db->fields, k);
			for (int i = 0; i < fu->count; i++)
			{
				sis_llutoa(fidx, key, 32, 10);
				sis_dynamic_field_json_to_struct(o + index * collect_->value->len, fu, fidx, key, jval);
				fidx++;
			}
		}
		index++;
		jval = jval->next;
	}

	sis_json_close(handle);
	return o;
}