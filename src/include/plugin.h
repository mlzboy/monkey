/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Monkey HTTP Daemon
 *  ------------------
 *  Copyright (C) 2001-2010, Eduardo Silva P. <edsiper@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef MK_PLUGIN_H
#define MK_PLUGIN_H

#include "request.h"
#include "memory.h"
#include "iov.h"
#include "socket.h"
#include "config.h"

#define MK_PLUGIN_LOAD "plugins.load"

#define MK_PLUGIN_ERROR -1      /* plugin execution error */
#define MK_PLUGIN_

/* Plugin: Core types */
#define MK_PLUGIN_CORE_PRCTX (0)
#define MK_PLUGIN_CORE_THCTX (1)

/* Plugin: Stages */
#define MK_PLUGIN_STAGE_10 (2)     /* Before server's loop */
#define MK_PLUGIN_STAGE_20 (4)     /* Accepted connection */
#define MK_PLUGIN_STAGE_30 (8)     /* Connection assigned */
#define MK_PLUGIN_STAGE_40 (16)     /* Object Handler */
#define MK_PLUGIN_STAGE_50 (32)    /* Request ended */
#define MK_PLUGIN_STAGE_60 (64)    /* Connection closed */

/* Plugin: Network type */
#define MK_PLUGIN_NETWORK_IO (128)
#define MK_PLUGIN_NETWORK_IP (256)

/* Return values */
#define MK_PLUGIN_RET_NOT_ME -1
#define MK_PLUGIN_RET_CONTINUE 100
#define MK_PLUGIN_RET_END 200
#define MK_PLUGIN_RET_CLOSE_CONX 300

/* Event return values */
#define MK_PLUGIN_RET_EVENT_NOT_ME -300

struct plugin_types
{
    /* core */
    struct plugin *core_prctx;
    struct plugin *core_thctx;

    /* stages */
    struct plugin *stage_00;
    struct plugin *stage_10;
    struct plugin *stage_20;
    struct plugin *stage_30;
    struct plugin *stage_40;
    struct plugin *stage_50;
    struct plugin *stage_60;

    /* network */
    struct plugin *network_io;
    struct plugin *network_ip;
};

struct plugin_list
{
    struct plugin *p;
    struct plugin_list *next;
};

struct plugin_list *plg_list;

struct plugin
{
    char *shortname;
    char *name;
    char *version;
    char *path;
    void *handler;
    int *types;

    /* Plugin external functions */
    int (*call_init) (void *api, char *confdir);
    int (*call_worker_init) ();
    int (*call_stage_10) ();
    int (*call_stage_20) (unsigned int,
                          struct sched_connection *, struct client_request *);
    int (*call_stage_30) (struct client_request *, struct request *);
    int (*call_stage_40) (struct plugin *, struct client_request *, struct request *);
    int (*call_stage_40_event_read) (struct client_request *, struct request *);
    int (*call_stage_40_event_write)(struct client_request *, struct request *);

    pthread_key_t thread_key;
    struct plugin *next;
};

struct plugin_api
{
    struct server_config *config;
    struct plugin_list *plugins;
    struct sched_list_node **sched_list;

    /* Exporting Functions */
    void *(*mem_alloc) (int);
    void *(*mem_alloc_z) (int);
    void *(*mem_free) (void *);
    void *(*str_build) (char **, unsigned long *, const char *, ...);
    void *(*str_dup) (const char *);
    void *(*str_search) (char *, char *);
    void *(*str_search_n) (char *, char *, int);
    void *(*str_copy_substr) (const char *, int, int);
    void *(*str_split_line) (const char *);
    void *(*file_to_buffer) (char *);
    void *(*file_get_info) (char *);
    void *(*header_send) (int,
                          struct client_request *,
                          struct request *, struct log_info *);
    void *(*iov_create) (int, int);
    void *(*iov_free) (struct mk_iov *);
    void *(*iov_add_entry) (struct mk_iov *, char *, int, mk_pointer, int);
    void *(*iov_set_entry) (struct mk_iov *, char *, int, int, int);
    void *(*iov_send) (int, struct mk_iov *, int);
    void *(*iov_print) (struct mk_iov *);
    void *(*pointer_set) (mk_pointer *, char *);
    void *(*pointer_print) (mk_pointer);
    void *(*plugin_load_symbol) (void *, char *);
    void *(*socket_cork_flag) (int, int);
    void *(*socket_set_tcp_nodelay) (int);
    void *(*socket_connect) (int, char *, int);
    void *(*socket_set_nonblocking) (int);
    void *(*socket_create) ();
    void *(*config_create) (char *);
    void *(*config_free) (struct mk_config *);
    void *(*config_getval) (struct mk_config *, char *, int);
    void *(*sched_get_connection) (struct sched_list_node *, int);
    void *(*event_add) (int, struct plugin *, struct client_request *, 
                        struct request *);
    void *(*event_socket_change_mode) (int, int);

#ifdef TRACE
    void *(*trace)();
#endif

};

typedef char mk_plugin_data_t[];
typedef int mk_plugin_stage_t;

/* Plugin events thread key */
pthread_key_t mk_plugin_event_k;

struct plugin_event {
    int socket;
    struct plugin *handler;
    struct client_request *cr;
    struct request *sr;
    struct plugin_event *next;
};

void mk_plugin_init();
int mk_plugin_stage_run(mk_plugin_stage_t stage,
                        unsigned int socket,
                        struct sched_connection *conx,
                        struct client_request *cr, struct request *sr);
void mk_plugin_worker_startup();

void mk_plugin_request_handler_add(struct request *sr, struct plugin *p);
void mk_plugin_request_handler_del(struct request *sr, struct plugin *p);

void mk_plugin_preworker_calls();

/* Plugins events interface */
int mk_plugin_event_add(int socket, struct plugin *handler,
                        struct client_request *cr, 
                        struct request *sr);
int mk_plugin_event_set_list(struct plugin_event *event);
struct plugin_event *mk_plugin_event_get_list();
int mk_plugin_event_socket_change_mode(int socket, int mode);

/* Plugins event handlers */
int mk_plugin_event_read(int socket);
int mk_plugin_event_write(int socket);
int mk_plugin_event_error(int socket);
int mk_plugin_event_close(int socket);
int mk_plugin_event_timeout(int socket);

void mk_plugin_register_to(struct plugin **st, struct plugin *p);
#endif
