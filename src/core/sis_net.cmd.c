﻿
#include <sis_net.cmd.h>
#include <sis_list.h>

static struct s_sis_method _sis_net_method_system[] = {
	{"auth",   SIS_NET_METHOD_SYSTEM, sis_net_cmd_auth, ""},
	{"sub",    SIS_NET_METHOD_SYSTEM, sis_net_cmd_sub, ""},
	{"unsub",  SIS_NET_METHOD_SYSTEM, sis_net_cmd_unsub, ""}, 
	{"pub",    SIS_NET_METHOD_SYSTEM, sis_net_cmd_pub, ""},  
};

s_sis_map_pointer *sis_net_command_create()
{
	int nums = sizeof(_sis_net_method_system) / sizeof(struct s_sis_method);
	s_sis_map_pointer *map = sis_map_pointer_create_v(sis_free_call);
	for (int i = 0; i < nums; i++)
	{
		s_sis_method *c = SIS_MALLOC(s_sis_method, c);
		memmove(c, &_sis_net_method_system[i], sizeof(s_sis_method));
		char key[128];
		sis_sprintf(key, 128, "%s.%s", c->style, c->name);
		sis_map_pointer_set(map, key, c);
	}
	return map;
}
void sis_net_command_destroy(s_sis_map_pointer *map_)
{
	sis_method_map_destroy(map_);
}

void sis_net_command_append(s_sis_net_class *sock_, s_sis_method *method_)
{
	if(!method_)
	{
		return ;
	}
	char key[128];
	sis_sprintf(key, 128, "%s.%s", method_->style, method_->name);
	sis_map_pointer_set(sock_->map_command, key, method_);
}
void sis_net_command_remove(s_sis_net_class *sock_, const char *name_, const char *style_)
{
	char key[128];
	sis_sprintf(key, 128, "%s.%s", style_, name_);
	sis_map_pointer_del(sock_->map_command, key);
}

////////////////////////////////////////////////////////////////////////////////
// command list
////////////////////////////////////////////////////////////////////////////////
#define SIS_NET_REPLY_SHORT_LEN 1024
bool sis_socket_send_reply_info(s_sis_net_class *sock_, int cid_,const char *in_)
{
	char str[SIS_NET_REPLY_SHORT_LEN];
	sis_sprintf(str, SIS_NET_REPLY_SHORT_LEN, "+%s\r\n", in_);
	if (sock_->connect_method == SIS_NET_IO_CONNECT)
	{
		return sis_socket_client_send(sock_->client, str, strlen(str));
	}
	else
	{
		return sis_socket_server_send(sock_->server, cid_, str, strlen(str));
	}
}
bool sis_socket_send_reply_error(s_sis_net_class *sock_, int cid_,const char *in_)
{
	char str[SIS_NET_REPLY_SHORT_LEN];
	sis_sprintf(str, SIS_NET_REPLY_SHORT_LEN, "-%s\r\n", in_);
	if (sock_->connect_method == SIS_NET_IO_CONNECT)
	{
		return  sis_socket_client_send(sock_->client, str, strlen(str));
	}
	else
	{
		return sis_socket_server_send(sock_->server, cid_, str, strlen(str));
	}
}
bool sis_socket_send_reply_buffer(s_sis_net_class *sock_, int cid_,const char *in_, size_t ilen_)
{
	if (sock_->connect_method == SIS_NET_IO_CONNECT)
	{
		return sis_socket_client_send(sock_->client, in_, ilen_);
	}
	else
	{
		return sis_socket_server_send(sock_->server, cid_, in_, ilen_);
	}
}
bool sis_socket_check_auth(s_sis_net_class *sock_, int cid_)
{
	char key[16];
	sis_sprintf(key, 16, "%d", cid_);
	s_sis_net_client *client = sis_map_pointer_get(sock_->sessions, key);
	if (!client->auth)
	{
		sis_socket_send_reply_error(sock_, cid_, "no auth.");
		return false;
	}
	return true;
}

void * sis_net_cmd_auth(void *sock_, void *mess_)
{
	s_sis_net_class *sock = (s_sis_net_class *)sock_;
	s_sis_net_message *mess = (s_sis_net_message *)mess_;

	if (sock->cb_auth)
	{
		if (!sock->cb_auth(sock_, mess->key, mess->argv))
		{
			sis_socket_send_reply_error(sock, mess->cid, "auth fail.");
			return SIS_METHOD_VOID_FALSE;
		}
	}
	char key[16];
	sis_sprintf(key, 16, "%d", mess->cid);
	s_sis_net_client *client = sis_map_pointer_get(sock->sessions, key);
	if (client)
	{
		client->auth = true;
	}
	sis_socket_send_reply_info(sock, mess->cid, SIS_REPLY_MSG_OK);
	return SIS_METHOD_VOID_TRUE;
}
void * sis_net_cmd_unsub(void *sock_, void *mess_)
{
	s_sis_net_class *sock = (s_sis_net_class *)sock_;
	s_sis_net_message *mess = (s_sis_net_message *)mess_;
	if(!sis_socket_check_auth(sock, mess->cid))
	{
		return SIS_METHOD_VOID_FALSE;
	}

	s_sis_struct_list *cids = (s_sis_struct_list *)sis_map_pointer_get(sock->sub_clients, mess->key);
	if (!cids)
	{
		return SIS_METHOD_VOID_FALSE;
	}
	for (int i = 0; i < cids->count; i++)
	{
		int cid = *(int *)sis_struct_list_get(cids, i);
		if (cid == mess->cid)
		{
			sis_struct_list_delete(cids, i, 1);
			sis_socket_send_reply_info(sock, mess->cid, SIS_REPLY_MSG_OK);
			return SIS_METHOD_VOID_TRUE;
		}
	}
	sis_socket_send_reply_error(sock, mess->cid, "no find sub key.");
	return SIS_METHOD_VOID_FALSE;
}

void * sis_net_cmd_sub(void *sock_, void *mess_)
{
	s_sis_net_class *sock = (s_sis_net_class *)sock_;
	s_sis_net_message *mess = (s_sis_net_message *)mess_;
	if(!sis_socket_check_auth(sock, mess->cid))
	{
		return SIS_METHOD_VOID_FALSE;
	}

	s_sis_struct_list *cids = (s_sis_struct_list *)sis_map_pointer_get(sock->sub_clients, mess->key);
	if (!cids)
	{
		cids = sis_struct_list_create(sizeof(int), NULL, 0);
		sis_map_pointer_set(sock->sub_clients, mess->key, cids);
	}
	for (int i = 0; i < cids->count; i++)
	{
		int cid = *(int *)sis_struct_list_get(cids, i);
		if (cid == mess->cid)
		{
			sis_socket_send_reply_error(sock, mess->cid, "already sub.");
			return SIS_METHOD_VOID_FALSE;
		}
	}
	sis_struct_list_push(cids, &mess->cid);
	sis_socket_send_reply_info(sock, mess->cid, SIS_REPLY_MSG_OK);
	return SIS_METHOD_VOID_TRUE;
}
void * sis_net_cmd_pub(void *sock_, void *mess_)
{
	s_sis_net_class *sock = (s_sis_net_class *)sock_;
	s_sis_net_message *mess = sis_net_message_clone((s_sis_net_message *)mess_);
	if(!sis_socket_check_auth(sock, mess->cid))
	{
		return SIS_METHOD_VOID_FALSE;
	}

	mess->style = SIS_NET_REPLY_ARRAY;
	mess->subpub = 1;
	s_sis_list_node *newnode = NULL;
	if (mess->argv)
	{
		newnode = sis_sdsnode_create(mess->argv, sis_sdslen(mess->argv));
	}
	s_sis_list_node *node = mess->argvs;
	while (node != NULL)
	{
		newnode = sis_sdsnode_push_node(newnode, node->value, sis_sdslen((s_sis_sds)node->value));
		node = node->next;
	};
	mess->rlist = newnode;

	s_sis_sds out = sock->cb_pack(sock, mess);

	if (!out)
	{
		sis_socket_send_reply_error(sock, mess->cid, "unknown format.");
		sis_net_message_destroy(mess);
		return (void *)0;
	}
	// 
	sis_socket_send_reply_info(sock, mess->cid, SIS_REPLY_MSG_OK);

	int64 oks = 0;
	s_sis_struct_list *cids = (s_sis_struct_list *)sis_map_pointer_get(sock->sub_clients, mess->key);
	if (cids)
	{
		for (int i = 0; i < cids->count; i++)
		{
			int cid = *(int *)sis_struct_list_get(cids, i);
			if (sis_socket_send_reply_buffer(sock, cid, out, sis_sdslen(out)))
			// if (_sis_net_class_sendto(sock_, cid, out, sis_sdslen(out)))
			{
				oks++;
			}
		}
	}
	printf("publish key =%lld\n",oks);
	sis_sdsfree(out);
	sis_net_message_destroy(mess);
	return (void *)oks;	
}
