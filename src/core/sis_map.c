﻿
#include "sis_map.h"
#include "sis_list.h"

// uint64_t _sis_dict_sds_hash(const void *key)
// {
// 	return sis_dict_hash_func((unsigned char *)key, sis_sdslen((char *)key));
// }

// uint64_t _sis_dict_sdscase_hash(const void *key)
// {
// 	return sis_dict_casehash_func((unsigned char *)key, sis_sdslen((char *)key));
// }
// int _sis_dict_sdscase_compare(void *privdata, const void *key1, const void *key2)
// {
// 	SIS_NOTUSED(privdata);
// 	return sis_strcasecmp((const char *)key1, (const char *)key2) == 0;
// }
// void *_sis_dict_sds_dup(void *privdata, const void *val)
// {
// 	SIS_NOTUSED(privdata);
// 	return sis_sdsnew(val);
// }
// void _sis_dict_buffer_free(void *privdata, void *val)
// {
// 	SIS_NOTUSED(privdata);
// 	sis_free(val);
// }
// void _sis_dict_sds_free(void *privdata, void *val)
// {
// 	SIS_NOTUSED(privdata);
// 	sis_sdsfree((s_sis_sds)val);
// }

uint64_t _sis_dict_str_hash(const void *key)
{
	return sis_dict_hash_func((unsigned char *)key, sis_strlen((char *)key));
}
uint64_t _sis_dict_strcase_hash(const void *key)
{
	return sis_dict_casehash_func((unsigned char *)key, sis_strlen((char *)key));
}
int _sis_dict_strcase_compare(const void *key1, const void *key2)
{
	return sis_strcasecmp((const char *)key1, (const char *)key2) == 0;
}
void *_sis_dict_sds_dup(const void *val)
{
	return sis_sdsnew(val);
}
void *_sis_dict_str_dup(const void *val)
{
	int size = sis_strlen((char *)val);
	char *o = sis_malloc(size + 1);
	sis_strcpy(o, size + 1, (char *)val);
	return o;
}
void _sis_dict_buffer_free(void *val)
{
	sis_free(val);
}
void _sis_dict_sds_free(void *val)
{
	sis_sdsfree((s_sis_sds)val);
}
void _sis_dict_str_free(void *val)
{
	sis_free(val);
}

s_sis_dict_type _sis_dict_type_buffer_s = {
	_sis_dict_strcase_hash,	   /* hash function */
	_sis_dict_str_dup,		   /* key dup */
	NULL,					   /* val dup */
	_sis_dict_strcase_compare, /* key compare */
	_sis_dict_str_free,		   /* key destructor */
	_sis_dict_buffer_free	   /* val destructor */
};
s_sis_dict_type _sis_dict_type_owner_free_val_s = {
	_sis_dict_strcase_hash,	/* hash function */
	_sis_dict_str_dup,		   /* key dup */
	NULL,					   /* val dup */
	_sis_dict_strcase_compare, /* key compare */
	_sis_dict_str_free,		   /* key destructor */
	NULL					   /* val destructor */
};
s_sis_dict_type _sis_dict_type_sds_s = {
	_sis_dict_strcase_hash,	/* hash function */
	_sis_dict_str_dup,		   /* key dup */
	_sis_dict_sds_dup,		   /* val dup */
	_sis_dict_strcase_compare, /* key compare */
	_sis_dict_str_free,		   /* key destructor */
	_sis_dict_sds_free		   /* val destructor */
};

//////////////////////////////////////////
//  s_sis_map_buffer 基础定义
///////////////////////////////////////////////

s_sis_map_buffer *sis_map_buffer_create()
{ //明确知道val的长度
	s_sis_map_buffer *map = sis_dict_create(&_sis_dict_type_buffer_s, NULL);
	return map;
};
void sis_map_buffer_destroy(s_sis_map_buffer *map_)
{
	sis_dict_destroy(map_);
};
void sis_map_buffer_clear(s_sis_map_buffer *map_)
{
	sis_dict_empty(map_, NULL);
};
void *sis_map_buffer_get(s_sis_map_buffer *map_, const char *key_)
{
	if (!map_)
	{
		return NULL;
	}
	s_sis_dict_entry *he = sis_dict_find(map_, key_);
	if (!he)
	{
		return NULL;
	}
	return sis_dict_getval(he);
};
void *sis_map_buffer_find(s_sis_map_buffer *map_, const char *key_)
{
	if (!map_)
	{
		return NULL;
	}
	s_sis_dict_entry *he = sis_dict_find(map_, key_);
	if (!he)
	{
		return NULL;
	}
	return sis_dict_getval(he);
};
int sis_map_buffer_set(s_sis_map_buffer *map_, const char *key_, void *val_)
{
	sis_dict_replace(map_, (void *)key_, val_);
	return 0;
}
//////////////////////////////////////////
//  s_sis_map_list 基础定义
//////////////////////////////////////////

s_sis_map_list *sis_map_list_create(void *vfree_)
{
	s_sis_map_list *o = SIS_MALLOC(s_sis_map_list, o);
	o->map = sis_map_int_create();
	o->list = sis_pointer_list_create();
	o->list->vfree = vfree_;
	return o;
}
void sis_map_list_destroy(s_sis_map_list *mlist_)
{
	sis_map_int_destroy(mlist_->map);
	sis_pointer_list_destroy(mlist_->list);
	sis_free(mlist_);
}
void sis_map_list_clear(s_sis_map_list *mlist_)
{
	sis_map_int_clear(mlist_->map);
	sis_pointer_list_clear(mlist_->list);
}
int sis_map_list_get_index(s_sis_map_list *mlist_, const char *key_)
{
	if (!mlist_ || !key_ || sis_map_list_getsize(mlist_) < 1)
	{
		return -1;
	}
	return sis_map_int_get(mlist_->map, key_);
}
void *sis_map_list_geti(s_sis_map_list *mlist_, int index_)
{
	return sis_pointer_list_get(mlist_->list, index_);
}

// void *sis_map_list_first(s_sis_map_list *mlist_)
// {
// 	for (int i = 0; i < mlist_->list->count; i++)
// 	{
// 		void *val = sis_pointer_list_get(mlist_->list, i);
// 		if (val)
// 		{
// 			return val;
// 		}
// 	}
// 	return NULL;
// }
// void *sis_map_list_next(s_sis_map_list *mlist_, void *node_)
// {
// 	int index = sis_pointer_list_indexof(mlist_, node_);
// 	if (index < 0)
// 	{
// 		return NULL;
// 	}
// 	for (int i = index + 1; i < mlist_->list->count; i++)
// 	{
// 		void *val = sis_pointer_list_get(mlist_->list, i);
// 		if (val)
// 		{
// 			return val;
// 		}
// 	}
// 	return NULL;
// }
void *sis_map_list_get(s_sis_map_list *mlist_, const char *key_)
{
	int index = sis_map_list_get_index(mlist_, key_);
	if (index < 0 || index > mlist_->list->count - 1)
	{
		return NULL;
	}
	return sis_pointer_list_get(mlist_->list, index);
}

// map有变化必须全部重索引
int sis_map_list_set(s_sis_map_list *mlist_, const char *key_, void *value_)
{
	int index = sis_map_list_get_index(mlist_, key_);
	// printf(" %s index = %d\n",key_, index);
	if (index >= 0)
	{
		// sis_map_int_set(mlist_->map, key_, index);
		sis_pointer_list_update(mlist_->list, index, value_);
	}
	else
	{
		sis_map_int_set(mlist_->map, key_, mlist_->list->count);
		sis_pointer_list_push(mlist_->list, value_);
		index = mlist_->list->count - 1;
	}
	return index;
}
int sis_map_list_getsize(s_sis_map_list *mlist_)
{
	return mlist_->list->count;
}

//////////////////////////////////////////
//  s_sis_map_int 基础定义
//////////////////////////////////////////
s_sis_map_pointer *sis_map_custom_create(s_sis_dict_type *type_)
{
	s_sis_map_pointer *map = sis_dict_create(type_, NULL);
	return map;
}

s_sis_map_pointer *sis_map_pointer_create()
{
	s_sis_map_pointer *map = sis_dict_create(&_sis_dict_type_owner_free_val_s, NULL);
	return map;
};

s_sis_map_pointer *sis_map_pointer_create_v(void *vfree_)
{
	s_sis_dict_type *type = SIS_MALLOC(s_sis_dict_type, type);
	memmove(type, &_sis_dict_type_owner_free_val_s, sizeof(s_sis_dict_type));
	s_sis_map_pointer *map = sis_dict_create(type, (void *)1);
	// s_sis_map_pointer *map = sis_dict_create(type, NULL);
	map->type->vfree = vfree_;
	return map;
};
void sis_map_pointer_destroy(s_sis_map_pointer *map_)
{
	sis_dict_empty(map_, NULL);
	if (map_->privdata != NULL)
	{
		sis_free(map_->type);
	}
	sis_free(map_);
	// sis_dict_destroy(map_);
};
void sis_map_pointer_del(s_sis_map_pointer *map_, const char *key_)
{
	sis_dict_delete(map_, key_);
}
//////////////////////////////////////////
//  s_sis_map_int 基础定义
//////////////////////////////////////////
s_sis_map_int *sis_map_int_create()
{
	s_sis_map_int *map = sis_dict_create(&_sis_dict_type_owner_free_val_s, NULL);
	return map;
};
int64 sis_map_int_get(s_sis_map_int *map_, const char *key_)
{
	s_sis_dict_entry *he;
	he = sis_dict_find(map_, key_);
	if (!he)
	{
		return -1;
	}
	return sis_dict_get_int(he);
};
int sis_map_int_set(s_sis_map_int *map_, const char *key_, int64 value_)
{
	return sis_dict_set_int(map_, (void *)key_, value_);
}
//////////////////////////////////////////
//  s_sis_map_sds 基础定义
//////////////////////////////////////////
s_sis_map_sds *sis_map_sds_create()
{
	s_sis_map_sds *map = sis_dict_create(&_sis_dict_type_sds_s, NULL);
	return map;
};
int sis_map_sds_set(s_sis_map_sds *map_, const char *key_, char *val_)
{
	sis_dict_replace(map_, (void *)key_, val_);
	return 0;
}

// int main()
// {
// 	safe_memory_start();
// 	char var1[128];
// 	sis_strcpy(var1, 128, "my is dingzhengdong.");
// 	char var2[128];
// 	sis_strcpy(var2, 128, "my is xuping.");
// 	char *outstr;
// 	{
// 		s_sis_map_buffer *map = sis_map_buffer_create();
// 		{
// 			char *str = sis_malloc(128);
// 			memmove(str, var1, 127);
// 			sis_map_buffer_set(map, "ding", str);
// 		}
// 		outstr = sis_map_buffer_get(map, "ding");
// 		printf("out : %s\n", outstr);

// 		{
// 			char *str = sis_malloc(128);
// 			memmove(str, var2, 127);
// 			sis_map_buffer_set(map, "ding", str);
// 		}
// 		outstr = sis_map_buffer_get(map, "ding");
// 		printf("out : %s\n", outstr);
// 		sis_map_buffer_destroy(map);
// 	}
// 	safe_memory_stop();
// 	return 0;
// }

// int main()
// {
// 	safe_memory_start();
// 	char var1[128];
// 	sis_strcpy(var1, 128, "my is dingzhengdong.");
// 	char var2[128];
// 	sis_strcpy(var2, 128, "my is xuping.");
// 	char *outstr;
// 	{
// 		s_sis_map_pointer *map = sis_map_pointer_create_v(sis_free_call);
// 		{
// 			char *str = sis_malloc(128);
// 			memmove(str, var1, 127);
// 			sis_map_pointer_set(map, "ding", str);
// 		}
// 		// {
// 		// 	char *str = sis_malloc(128);
// 		// 	memmove(str, var1, 127);
// 		// 	sis_map_pointer_set(map, "xu", str);
// 		// }
// 		// sis_map_pointer_del(map, "xu");
// 		outstr = sis_map_pointer_get(map, "ding");
// 		printf("out : %s\n", outstr);
// 		{
// 			char *str = sis_malloc(128);
// 			memmove(str, var2, 127);
// 			sis_map_pointer_set(map, "ding", str);
// 		}
// 		outstr = sis_map_pointer_get(map, "ding");
// 		printf("out : %s\n", outstr);
// 		sis_map_pointer_destroy(map);
// 	}
// 	safe_memory_stop();
// 	return 0;
// }

// int main()
// {
// 	safe_memory_start();
// 	char var1[128];
// 	sis_strcpy(var1, 128, "my is dingzhengdong.");
// 	char var2[128];
// 	sis_strcpy(var2, 128, "my is xuping.");
// 	char *outstr;
// 	{
// 		s_sis_map_sds *map = sis_map_sds_create();
// 		sis_map_sds_set(map, "ding", var1);
// 		sis_map_sds_set(map, "xu", var1);
// 		outstr = sis_map_sds_get(map, "ding");
// 		printf("out : %s\n", outstr);
// 		sis_map_sds_set(map, "ding", var2);
// 		outstr = sis_map_sds_get(map, "ding");
// 		printf("out : %s\n", outstr);
// 		sis_map_sds_destroy(map);
// 	}
// 	safe_memory_stop();
// 	return 0;
// }
#if 0

int main()
{
	safe_memory_start();
	s_sis_map_buffer *map = sis_map_buffer_create();
	char key[128];
	sis_sprintf(key, 128, "ask.%s", "dingzhengdong");

	char *str = sis_malloc(200);
	sis_map_buffer_set(map, key, str);
	printf("in %p %p\n",map, str);
	char *sss = sis_map_buffer_get(map, key);
	printf("out %p %p\n",map, sss);

	str = sis_malloc(100);
	sis_map_buffer_set(map, key, str);
	printf("in %p %p\n",map, str);
	sss = sis_map_buffer_get(map, key);
	printf("out %p %p\n",map, sss);

	str = sis_malloc(100);
	sis_map_buffer_set(map, key, str);
	printf("in %p %p\n",map, str);
	sss = sis_map_buffer_get(map, key);
	printf("out %p %p\n",map, sss);

	for(size_t i = 0; i < 1000000; i++)
	{
		str = sis_malloc(rand() % 200);		
		sis_map_buffer_set(map, key, str);	
		printf("rand %d %p\n",(int)i, str);	
		sss = sis_map_buffer_get(map, key);
		printf("out %p %p\n",map, sss);
	}
	// str = sis_malloc(rand() % 100);	
	sis_map_buffer_destroy(map);
	safe_memory_stop();
}
#endif
