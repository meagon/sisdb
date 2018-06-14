
#include "sts_list.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  s_sts_struct_list
//
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

s_sts_struct_list *sts_struct_list_create(int len_, void *in_, int inlen_)
{
	// printf("new list...%d\n", len_);
	if (len_ < 1)
	{
		return NULL;
	}
	s_sts_struct_list *sbl = (s_sts_struct_list *)sts_malloc(sizeof(s_sts_struct_list));
	sbl->len = len_;
	sbl->maxcount = 0;
	sbl->count = 0;
	sbl->buffer = NULL;
	sbl->free = NULL;
	sbl->mode = STRUCT_LIST_NORMAL;
	if (in_ && inlen_ > 0)
	{
		sts_struct_list_set(sbl, in_, inlen_);
	}
	return sbl;
}
void sts_struct_list_destroy(s_sts_struct_list *list_)
{
	sts_struct_list_clear(list_);
	if (list_->buffer)
	{
		sts_free(list_->buffer);
	}
	list_->buffer = NULL;
	list_->maxcount = 0;
	sts_free(list_);
}
void sts_struct_list_clear(s_sts_struct_list *list_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		char **ptr = (char **)list_->buffer;
		for (int i = 0; i < list_->count; i++)
		{
			if (list_->free)
			{
				list_->free(ptr[i]);
				ptr[i] = NULL;
			}
		}
	}
	list_->count = 0;
}
void struct_list_grow(s_sts_struct_list *list_, int len_)
{
	if (len_ < list_->maxcount)
	{
		return;
	}
	int maxlen = len_;
	if (len_ < 8)
	{
		maxlen = 8;
	}
	else if (len_ >= 8 && len_ < 64)
	{
		maxlen = 64;
	}
	else if (len_ >= 64 && len_ < 256)
	{
		maxlen = 256;
	}
	else
	{
		maxlen = len_ + BUFFLIST_STEP_ROW;
	}

	// void *buffer = sts_malloc(maxlen * list_->len);
	// memmove(buffer, list_->buffer, list_->maxcount*list_->len);
	// sts_free(list_->buffer);
	// list_->buffer = buffer;
	list_->buffer = sts_realloc(list_->buffer, maxlen * list_->len);
	list_->maxcount = maxlen;
}
void struct_list_setsize(s_sts_struct_list *list_, int len_)
{
	if (len_ < list_->maxcount)
	{
		return;
	}
	// void *buffer = sts_malloc(len_ * list_->len);
	// memmove(buffer, list_->buffer, list_->maxcount*list_->len);
	// sts_free(list_->buffer);
	// list_->buffer = buffer;
	list_->buffer = sts_realloc(list_->buffer, len_ * list_->len);
	list_->maxcount = len_;
}

int sts_struct_list_push(s_sts_struct_list *list_, void *in_)
{
	struct_list_grow(list_, list_->count + 1);
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		char **ptr = (char **)list_->buffer;
		ptr[list_->count] = (char *)in_;
	}
	else
	{
		memmove((char *)list_->buffer + (list_->count * list_->len), in_, list_->len);
	}
	list_->count++;
	return list_->count;
}
int sts_struct_list_update(s_sts_struct_list *list_, int index_, void *in_)
{
	if (index_ >= 0 && index_ < list_->count)
	{
		if (list_->mode == STRUCT_LIST_POINTER)
		{
			char **ptr = (char **)list_->buffer;
			if (list_->free)
			{
				list_->free(ptr[index_]);
			}
			ptr[index_] = (char *)in_;
		}
		else
		{
			// if (index_==0)
			// printf("---2---value =%p %p len=%d\n", list_->buffer, list_, list_->len);
			memmove((char *)list_->buffer + (index_ * list_->len), in_, list_->len);
		}
		return index_;
	}
#if 0
	else
	{
		if (index_ < 0) {
			index_ = list_->count;
		}
		struct_list_grow(list_, index_ + 1);    // 这里可能会跳跃最后一条记录 
		if (list_->mode == STRUCT_LIST_POINTER) 
		{
			char **ptr = (char **)list_->buffer;
			ptr[index_] = (char *)in_;
		}
		else
		{
			memmove((char *)list_->buffer + (index_ * list_->len), in_, list_->len);
		}
		list_->count = index_ + 1;
	}
#endif
	return -1;
}
int sts_struct_list_insert(s_sts_struct_list *list_, int index_, void *in_)
{
	if (list_->count < 1)
	{
		return sts_struct_list_push(list_, in_);
	}
	if (index_ < 0 || index_ > list_->count - 1)
	{
		return -1;
	}
	struct_list_grow(list_, list_->count + 1);
	memmove((char *)list_->buffer + ((index_ + 1) * list_->len), (char *)list_->buffer + (index_ * list_->len),
			(list_->count - index_) * list_->len);

	if (list_->mode == STRUCT_LIST_POINTER)
	{
		char **ptr = (char **)list_->buffer;
		ptr[index_] = (char *)in_;
	}
	else
	{
		memmove((char *)list_->buffer + (index_ * list_->len), in_, list_->len);
	}
	list_->count++;
	return index_;
}
void *sts_struct_list_first(s_sts_struct_list *list_)
{
	return sts_struct_list_get(list_, 0);
}
void *sts_struct_list_last(s_sts_struct_list *list_)
{
	return sts_struct_list_get(list_, list_->count - 1);
}
void *sts_struct_list_get(s_sts_struct_list *list_, int index_)
{
	char *rtn = NULL;
	if (index_ >= 0 && index_ < list_->count)
	{
		if (list_->mode == STRUCT_LIST_POINTER)
		{
			char **ptr = (char **)list_->buffer;
			rtn = ptr[index_];
		}
		else
		{
			rtn = (char *)list_->buffer + (index_ * list_->len);
		}
	}
	return rtn;
}
void *sts_struct_list_next(s_sts_struct_list *list_, void *current_, int offset)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return current_;
	}
	char *rtn = (char *)current_ + offset * list_->len;
	if (rtn >= (char *)list_->buffer && rtn <= (char *)list_->buffer + (list_->count - 1) * list_->len)
	{
		return rtn;
	}
	else
	{
		return NULL;
	}
}

int sts_struct_list_set(s_sts_struct_list *list_, void *in_, int inlen_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return 0;
	}
	int count = inlen_ / list_->len;
	struct_list_setsize(list_, count);
	if (in_)
	{
		memmove(list_->buffer, in_, inlen_);
	}
	else
	{
		memset(list_->buffer, 0, count * list_->len);
	}
	list_->count = count;
	return count;
}

void sts_struct_list_limit(s_sts_struct_list *list_, int limit_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return;
	}
	if (limit_ < 1 || limit_ > list_->count)
	{
		return;
	}
	int offset = list_->count - limit_;
	memmove(list_->buffer, (char *)list_->buffer + (offset * list_->len), limit_ * list_->len);
	list_->count = limit_;
}
int sts_struct_list_clone(s_sts_struct_list *src_, s_sts_struct_list *list_, int limit_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return 0;
	}
	int count;
	int offset;
	if (limit_ < 1 || limit_ > src_->count)
	{
		count = src_->count;
	}
	else
	{
		count = limit_;
	}
	offset = src_->count - count;
	sts_struct_list_set(list_, (char *)src_->buffer + (offset * src_->len), count * list_->len);
	return count;
}
int sts_struct_list_pack(s_sts_struct_list *list_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return 0;
	}
	if (!list_)
	{
		return 0;
	}
	char *tmp = (char *)sts_malloc(list_->count * list_->len);
	memmove(tmp, list_->buffer, list_->count * list_->len);
	sts_free(list_->buffer);
	list_->buffer = tmp;
	list_->maxcount = list_->count;
	return list_->count;
}
int sts_struct_list_delete(s_sts_struct_list *list_, int start_, int count_)
{
	if (start_ < 0 || count_ < 1 || start_ + count_ > list_->count)
	{
		return 0;
	}

	if (list_->mode == STRUCT_LIST_POINTER)
	{
		char **ptr = (char **)list_->buffer;
		for (int i = start_; i < start_ + count_; i++)
		{
			if (list_->free)
			{
				list_->free(ptr[i]);
			}
		}
	}
	memmove((char *)list_->buffer + (start_ * list_->len), (char *)list_->buffer + ((start_ + count_) * list_->len),
			(list_->count - count_ - start_) * list_->len);

	list_->count = list_->count - count_;
	return count_;
}

int sts_struct_list_pto_recno(s_sts_struct_list *list_, void *p_)
{
	if (list_->mode == STRUCT_LIST_POINTER)
	{
		return 0;
	}
	if ((char *)p_ < (char *)list_->buffer ||
		(char *)p_ > ((char *)list_->buffer + list_->len * (list_->count - 1)))
	{
		return -1;
	}
	return (int)(((char *)p_ - (char *)list_->buffer) / list_->len);
}
///////////////////////////////////////////////////////////////////////////
//------------------------s_pointer_list --------------------------------//
//  存储指针的列表,依赖于struct_list,记录长度为sizeof(char *)
///////////////////////////////////////////////////////////////////////////
s_sts_struct_list *sts_pointer_list_create()
{
	s_sts_struct_list *sbl = (s_sts_struct_list *)sts_malloc(sizeof(s_sts_struct_list));
	sbl->len = sizeof(char *);
	sbl->maxcount = 0;
	sbl->count = 0;
	sbl->buffer = NULL;
	sbl->free = NULL;
	sbl->mode = STRUCT_LIST_POINTER;
	return sbl;
}
int sts_pointer_list_indexof(s_sts_struct_list *list_, void *in_)
{
	if (list_->mode != STRUCT_LIST_POINTER)
		return -1;
	char **ptr = (char **)list_->buffer;
	for (int i = 0; i < list_->count; i++)
	{
		if (ptr[i] == in_)
		{
			return i;
		}
	}
	return -1;
}
int sts_pointer_list_find_and_update(s_sts_struct_list *list_, void *finder_, void *in_)
{
	if (list_->mode != STRUCT_LIST_POINTER)
		return -1;
	int index = sts_pointer_list_indexof(list_, finder_);
	return sts_struct_list_update(list_, index, in_);
}
int sts_pointer_list_find_and_delete(s_sts_struct_list *list_, void *finder_)
{
	if (list_->mode != STRUCT_LIST_POINTER)
		return -1;
	char **ptr = (char **)list_->buffer;
	for (int i = 0; i < list_->count; i++)
	{
		if (ptr[i] == finder_)
		{
			if (list_->free)
			{
				list_->free(ptr[i]);
			}
			memmove(&ptr[i], &ptr[i + 1], (list_->count - 1 - i) * list_->len);
			list_->count--;
			return i;
		}
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////
//------------------------s_sts_string_list --------------------------------//
//  存储不定长字符串的列表，
///////////////////////////////////////////////////////////////////////////
s_sts_string_list *sts_string_list_create_r() //只读
{
	s_sts_string_list *l = (s_sts_string_list *)sts_malloc(sizeof(s_sts_string_list));
	l->strlist = sts_pointer_list_create();
	l->permissions = STRING_LIST_RD;
	return l;
}
void sts_string_listfree(void *p)
{
	sts_free(p);
}
s_sts_string_list *sts_string_list_create_w() //读写
{
	s_sts_string_list *l = (s_sts_string_list *)sts_malloc(sizeof(s_sts_string_list));
	l->strlist = sts_pointer_list_create();
	l->permissions = STRING_LIST_WR;
	l->strlist->free = sts_string_listfree;
	return l;
}
void sts_string_list_destroy(s_sts_string_list *list_)
{
	sts_string_list_clear(list_);
	if (list_->permissions == STRING_LIST_WR)
	{
		sts_pointer_list_destroy(list_->strlist);
	}
	sts_free(list_);
}
void sts_string_list_clear(s_sts_string_list *list_)
{
	if (list_->permissions == STRING_LIST_RD && list_->m_ptr_r)
	{
		sts_free(list_->m_ptr_r);
		list_->m_ptr_r = NULL;
	}
	sts_pointer_list_clear(list_->strlist);
}
// read & write
int sts_string_list_load(s_sts_string_list *list_, const char *in_, size_t inlen_, const char *sign)
{
	sts_string_list_clear(list_);

	if (strlen(in_) == 0)
	{
		return 0;
	}
	char *token = NULL;
	char *src = (char *)sts_malloc(inlen_ + 1);
	sts_strncpy(src, inlen_ + 1, in_, inlen_);

	if (list_->permissions == STRING_LIST_RD)
	{
		list_->m_ptr_r = src;
	}
	char *ptr = src;
	char *des = NULL;
	size_t len;
	while ((token = strsep(&ptr, sign)) != NULL)
	{
		sts_trim(token);
		if (list_->permissions == STRING_LIST_WR)
		{
			len = strlen(token);
			des = (char *)sts_malloc(len + 1);
			sts_strncpy(des, len + 1, token, len);
		}
		else
		{
			des = token;
		}
		sts_pointer_list_push(list_->strlist, des);
	}
	if (list_->permissions == STRING_LIST_WR)
	{
		sts_free(src);
	}
	return list_->strlist->count;
}
const char *sts_string_list_get(s_sts_string_list *list_, int index_)
{
	return (const char *)sts_pointer_list_get(list_->strlist, index_);
}
int sts_string_list_getsize(s_sts_string_list *list_)
{
	return list_->strlist->count;
}

int sts_string_list_indexof(s_sts_string_list *list_, const char *in_)
{
	if (!in_)
		return -1;
	for (int i = 0; i < list_->strlist->count; i++)
	{
		if (!strcmp(sts_string_list_get(list_, i), in_))
			return i;
	}
	return -1;
}
int sts_string_list_indexofcase(s_sts_string_list *list_, const char *in_)
{
	if (!in_)
		return -1;
	for (int i = 0; i < list_->strlist->count; i++)
	{
		if (!strcasecmp(sts_string_list_get(list_, i), in_))
			return i;
	}
	return -1;
}
int sts_string_list_update(s_sts_string_list *list_, int index_, const char *in_, size_t inlen)
{
	if (list_->permissions != STRING_LIST_WR)
		return -1;

	char *str = (char *)sts_malloc(inlen + 1);
	sts_strncpy(str, inlen + 1, in_, inlen);
	int index = sts_pointer_list_update(list_->strlist, index_, str);
	if (index < 0)
		sts_free(str);
	return index;
}
int sts_string_list_find_and_update(s_sts_string_list *list_, char *finder_, const char *in_, size_t inlen)
{
	if (list_->permissions != STRING_LIST_WR)
		return -1;

	char *str = (char *)sts_malloc(inlen + 1);
	sts_strncpy(str, inlen + 1, in_, inlen);
	int index = sts_pointer_list_find_and_update(list_->strlist, finder_, str);
	if (index < 0)
		sts_free(str);
	return index;
}
int sts_string_list_insert(s_sts_string_list *list_, int index_, const char *in_, size_t inlen)
{
	if (list_->permissions != STRING_LIST_WR)
	{
		return -1;
	}

	char *str = (char *)sts_malloc(inlen + 1);
	sts_strncpy(str, inlen + 1, in_, inlen);
	int index = sts_pointer_list_insert(list_->strlist, index_, str);
	if (index < 0)
	{
		sts_free(str);
	}
	return index;
}
int sts_string_list_delete(s_sts_string_list *list_, int index_)
{
	return sts_pointer_list_delete(list_->strlist, index_, 1);
}
int sts_string_list_find_and_delete(s_sts_string_list *list_, const char *finder_)
{
	return sts_pointer_list_find_and_delete(list_->strlist, (void *)finder_);
}
int sts_string_list_push(s_sts_string_list *list_, const char *in_, size_t inlen)
{
	if (list_->permissions != STRING_LIST_WR)
		return -1;

	char *str = (char *)sts_malloc(inlen + 1);
	sts_strncpy(str, inlen + 1, in_, inlen);
	return sts_pointer_list_push(list_->strlist, str);
}
void sts_string_list_limit(s_sts_string_list *list_, int limit_)
{
	if (limit_ < 1 || (!list_->strlist) || limit_ > list_->strlist->count)
	{
		return;
	}
	int offset = list_->strlist->count - limit_;
	sts_pointer_list_delete(list_->strlist, 0, offset);
}

#if 0
///////test
#include <stdio.h>
//#include <malloc.h>
int char_to_int(void *str1, void *str2)
{
	printf("str1=%s %p %c\n", (char *)str1, (char *)str1, *(char *)(str1 + 1));
	printf("str2=%s %p %c\n", (char *)str2, (char *)str2, *(char *)(str2 + 1));
	return 0;
	char buff[100];// = (char *)malloc(100);
	char **ptr = (char **)buff;
	printf("buff = %p, ptr=%p %p %p\n", buff, ptr, &ptr[0], &ptr[1]);
	//ptr[0]=(char *)str1;
	memmove((char *)buff, &str1, sizeof(void *));
	ptr[1] = (char *)str2;
	printf("ptr=%p %p address=%p %p\n", ptr[0], ptr[1], &ptr[0], &ptr[1]);

	printf("ptr1 = %s,%s \n", ptr[0], ptr[0]);
	printf("ptr2 = %s,%s \n", ptr[1], ptr[1]);
	//free(buff);
	return 1;
}
int main1(void) {
	// your code goes here
	char *str[] = { "SH", "SZ" };
	char sss[100];
	sss[0] = '5'; sss[1] = '6'; sss[2] = 0;
	int ii = char_to_int(sss, str[0]);
	printf("str=%s %p %p  ii=%d\n", str[0], str[0], sss, ii);
	return 0;
}
int main(void) {
	s_sts_string_list *list = sts_string_list_create_w();
	sts_string_list_insert(list, 0, "1", 1);
	sts_string_list_push(list,  "2", 1);
	sts_string_list_push(list,  "3", 1);
	sts_string_list_push(list,  "4", 1);
	sts_string_list_insert(list, 1, "5", 1);
	for (int i=0;i<sts_string_list_getsize(list);i++) {
		printf("%s\n",sts_string_list_get(list, i));
	}

	sts_string_list_destroy(list);
	return 0;
}
#endif
