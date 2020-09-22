﻿
#include <sis_memory.h>
#include <sis_math.h>

s_sis_memory *sis_memory_create()
{
	s_sis_memory *m = (s_sis_memory *)sis_malloc(sizeof(s_sis_memory));
	m->buffer = (char *)sis_malloc(255);
	m->size = 0;
	m->maxsize = 255;
	m->offset = 0;
	return m;
}
void sis_memory_destroy(s_sis_memory *m_)
{
	if (m_->buffer)
	{
		sis_free(m_->buffer);
	}
	sis_free(m_);
}
void sis_memory_clone(s_sis_memory *src_, s_sis_memory *des_)
{
	sis_memory_cat(des_, sis_memory(src_), sis_memory_get_size(src_));
}

// void sis_memory_pack(s_sis_memory *m_)
// {
// 	if (m_->val > m_->buffer)
// 	{
// 		m_->size = sis_memory_get_size(m_);
// 		memmove(m_->buffer, m_->val, m_->size);
// 		m_->val = m_->buffer;
// 	}
// }
void sis_memory_clear(s_sis_memory *m_)
{
	m_->size = 0;
	m_->offset = 0;
}
void sis_memory_pack(s_sis_memory *m_)
{
	if (m_->offset <= 0)
	{
		return;
	}
	if (m_->offset > m_->size - 1)
	{
		m_->size = 0;
		m_->offset = 0;
	}
	else
	{
		m_->size = sis_memory_get_size(m_);
		memmove(m_->buffer, m_->buffer + m_->offset, m_->size);
		m_->offset = 0;
	}
}
void sis_memory_jumpto(s_sis_memory *m_, size_t off_)
{
	if (off_ < m_->size)
	{
		m_->offset = off_;
	}
}
size_t sis_memory_get_address(s_sis_memory *m_)
{
	return m_->offset;
}
char *sis_memory(s_sis_memory *m_)
{
	if (!m_->buffer)
	{
		return NULL;
	}
	return m_->buffer + m_->offset;
}
void sis_memory_grow(s_sis_memory *m_, size_t nsize_)
{
	if (nsize_ > m_->maxsize)
	{
		size_t grow = sis_min(SIS_MEMORY_SIZE, (nsize_ + m_->maxsize) / 2);
		m_->maxsize = nsize_ + grow;
		m_->buffer = (char *)sis_realloc(m_->buffer, m_->maxsize);
	}	
}
size_t sis_memory_cat(s_sis_memory *m_, char *in_, size_t ilen_)
{
	if (ilen_ > 0)
	{
		sis_memory_grow(m_, ilen_ + m_->size);
		memmove(m_->buffer + m_->size, in_, ilen_);
		m_->size += ilen_;
	}
	return ilen_;
}
size_t sis_memory_cat_int(s_sis_memory *m_, int in_)
{
	sis_memory_grow(m_, sizeof(int) + m_->size);
	memmove(m_->buffer + m_->size, &in_, sizeof(int));
	m_->size += sizeof(int);
	return sizeof(int);
}
int sis_memory_get_int(s_sis_memory *m_)
{
	int o = 0;
	memmove(&o, sis_memory(m_), sizeof(int));
	sis_memory_move(m_, sizeof(int));
	return o;
}

//////////////////
//
/////////////////
size_t sis_memory_cat_byte(s_sis_memory *m_, int64 in_, int bytes_)
{
	if (bytes_ == 1)
	{
		sis_memory_grow(m_, bytes_ + m_->size);
		int8 in = (int8)in_;
		memmove(m_->buffer + m_->size, &in, bytes_);
		m_->size += bytes_;
	}
	if (bytes_ == 2)
	{
		sis_memory_grow(m_, bytes_ + m_->size);
		int16 in = (int16)in_;
		memmove(m_->buffer + m_->size, &in, bytes_);
		m_->size += bytes_;
	}
	if (bytes_ == 4)
	{
		sis_memory_grow(m_, bytes_ + m_->size);
		int32 in = (int32)in_;
		memmove(m_->buffer + m_->size, &in, bytes_);
		m_->size += bytes_;
	}
	if (bytes_ == 8)
	{
		sis_memory_grow(m_, bytes_ + m_->size);
		int64 in = (int64)in_;
		memmove(m_->buffer + m_->size, &in, bytes_);
		m_->size += bytes_;
	}
	return bytes_;
}
int64 sis_memory_get_byte(s_sis_memory *m_, int bytes_)
{
	if (bytes_ == 1|| bytes_ == 2 || bytes_ == 4 || bytes_ == 8)
	{
		int64 o = 0;
		memmove(&o, sis_memory(m_), bytes_);
		sis_memory_move(m_, bytes_);
		return o;
	}
	return 0;
}

size_t sis_memory_cat_ssize(s_sis_memory *s_, size_t in_)
{
	if (in_ > 0xFFFFFFFF)
	{
		sis_memory_cat_byte(s_, 254, 1);
		sis_memory_cat_byte(s_, in_, 8);
		return 9;
	} else if (in_ > 0xFFFF + 250)
	{
		sis_memory_cat_byte(s_, 253, 1);
		sis_memory_cat_byte(s_, in_, 4);
		return 5;
	} else if (in_ > 0xFF + 250)
	{
		sis_memory_cat_byte(s_, 252, 1);
		sis_memory_cat_byte(s_, in_ - 250, 2);
		return 3;
	} else if (in_ > 250)
	{
		sis_memory_cat_byte(s_, 251, 1);
		sis_memory_cat_byte(s_, in_ - 250, 1);
		return 2;
	}
	// else
	{
		sis_memory_cat_byte(s_, in_, 1);
	}
	return 1;
}

size_t sis_memory_get_ssize(s_sis_memory *s_)
{
	int64 dw = sis_memory_get_byte(s_, 1);
	if (dw == 254||dw == 255)
	{
		dw = sis_memory_get_byte(s_, 8);
	}
	else if (dw == 253)
	{
		dw = sis_memory_get_byte(s_, 4);
	}
	else if (dw == 252) 
	{
		dw = sis_memory_get_byte(s_, 2) + 250;
	}
	else if (dw == 251) 
	{
		dw = sis_memory_get_byte(s_, 1) + 250;	
	}
	return (size_t)dw;
}
bool sis_memory_try_ssize(s_sis_memory *s_)
{
	if (sis_memory_get_size(s_) < 1)
	{
		return false;
	}
	uint8 dw;   
	memmove(&dw, sis_memory(s_), 1);
	switch (dw)
	{
	case 254:
	case 255:
		if (sis_memory_get_size(s_) > 8)
		{
			return true;
		}
		break;	
	case 253:
		if (sis_memory_get_size(s_) > 4)
		{
			return true;
		}
		break;	
	case 252:
		if (sis_memory_get_size(s_) > 2)
		{
			return true;
		}
		break;	
	case 251:
		if (sis_memory_get_size(s_) > 1)
		{
			return true;
		}
		break;	
	default:
		return true;
	}
	return false;
}

size_t sis_memory_cat_double(s_sis_memory *m_, double in_)
{
	sis_memory_grow(m_, sizeof(double) + m_->size);
	memmove(m_->buffer + m_->size, &in_, sizeof(double));
	m_->size += sizeof(double);
	return sizeof(double);
}

double sis_memory_get_double(s_sis_memory *m_)
{
	double o = 0.0;
	memmove(&o, sis_memory(m_), sizeof(double));
	sis_memory_move(m_, sizeof(double));
	return o;
}

size_t sis_memory_readfile(s_sis_memory *m_, s_sis_file_handle fp_, size_t len_)
{
	char *mem = (char *)sis_malloc(len_ + 1);
	size_t bytes = sis_file_read(fp_, mem, len_);
	if (bytes <= 0)
	{
		sis_free(mem);
		return 0;
	}
	sis_memory_pack(m_);

	sis_memory_grow(m_, bytes + m_->size);
	memmove(m_->buffer + m_->size, mem, bytes);
	m_->size += bytes;
	// m_->buffer[m_->size] = 0;

	sis_free(mem);

	// return sis_memory_get_size(m_);
	// 这里应该返回实际读的字符，
	return bytes;
}

size_t sis_memory_read(s_sis_memory *m_, s_sis_handle fp_, size_t len_)
{
	if (fp_ < 0)
	{
		return 0;
	}
	char *mem = (char *)sis_malloc(len_ + 1);
	size_t bytes = sis_read(fp_, mem, len_);
	if (bytes <= 0)
	{
		sis_free(mem);
		return 0;
	}
	sis_memory_pack(m_);

	sis_memory_grow(m_, bytes + m_->size);
	memmove(m_->buffer + m_->size, mem, bytes);
	m_->size += bytes;
	// m_->buffer[m_->size] = 0;

	sis_free(mem);

	// return sis_memory_get_size(m_);
	// 这里应该返回实际读的字符，
	return bytes;
}
size_t sis_memory_get_size(s_sis_memory *m_)
{
	return (m_->size - m_->offset);
}
// 得到当前可写的剩余空间
size_t sis_memory_get_freesize(s_sis_memory *m_)
{
	return (m_->maxsize - m_->size);
}

void sis_memory_set_size(s_sis_memory *m_, size_t len_)
{
	// printf("set size :%zu -- %zu\n", len_, m_->offset);
	sis_memory_set_maxsize(m_, len_ + m_->offset);
	m_->size = len_ + m_->offset;
}
size_t sis_memory_get_maxsize(s_sis_memory *m_)
{
	return m_->maxsize;
}
size_t sis_memory_set_maxsize(s_sis_memory *m_, size_t len_)
{
	// printf("%zu -- %zu\n", len_, m_->maxsize);
	if (len_ > m_->maxsize)
	{
		m_->maxsize = len_ + 256;
		m_->buffer = (char *)sis_realloc(m_->buffer, m_->maxsize);
	}
	return m_->maxsize;
}

static char *_seek_new_line(char *s, size_t len) {
    int pos = 0;
    int _len = len - 1;

    /* position should be < len-1 because the character at "pos" should be
     * followed by a \n. Note that strchr cannot be used because it doesn't
     * allow to search a limited length and the buffer that is being searched
     * might not have a trailing NULL character. */
    while (pos < _len) {
        while(pos < _len && s[pos] != '\r') pos++;
        if (pos==_len) {
            /* Not found. */
            return NULL;
        } else {
            if (s[pos + 1] == '\n') {
                /* found. */
                return s + pos;
            } else {
                /* continue searching. */
                pos++;
            }
        }
    }
    return NULL;
}
char *sis_memory_read_line(s_sis_memory *m_, size_t *len_) 
{
    char *p, *s;
    size_t len;

    p = sis_memory(m_);
    s = _seek_new_line(p, sis_memory_get_size(m_));
    if (s != NULL) 
	{
        len = s - sis_memory(m_);
        if (len_) *len_ = len;
        return p;
    }
    return NULL;
}
size_t sis_memory_get_line_sign(s_sis_memory *m_)
{
	if (!m_)
	{
		return 0;
	}
	char *ptr = sis_memory(m_);
	size_t len = 0;
	while (*ptr && (unsigned char)*ptr != '\n')
	{
		if ((m_->offset + len) < m_->size - 1)
		{
			ptr++;
			len++;
		}
		else
		{
			// len = 0;
			return 0;
		}
	}
	if ((unsigned char)*ptr == '\n')
	{
		len++;
	} // skip return key
	return len;
}

void sis_memory_move(s_sis_memory *m_, size_t len_)
{
	if ((m_->offset + len_) <= m_->size)
	{
		m_->offset += len_;
	}
	else
	{
		m_->offset = m_->size;
	}
}