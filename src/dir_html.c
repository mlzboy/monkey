/* -*- Mode: C; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 8 -*- */

/*  Monkey HTTP Daemon
 *  ------------------
 *  Copyright (C) 2001-2008, Eduardo Silva P.
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/***********************************************/ 
/* Modulo dir_html.c written by Daniel R. Ome */
/***********************************************/

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <time.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "monkey.h"
#include "http.h"
#include "http_status.h"
#include "str.h"
#include "memory.h"
#include "utils.h"
#include "config.h"
#include "method.h"
#include "socket.h"
#include "dir_html.h"
#include "header.h"
#include "file.h"
#include "iov.h"

#define  DIRECTORIO		  "     -"

/* Longitud de la cadena */
#define  MAX_LEN_STR		 30

/* Longitud de la fecha y hora */
#define  MAX_TIME			 17

/* Longitud del tama�o */
#define  MAX_SIZE			  6

/* Incremento */
#define  GROW				  100

#define  SPACE				 ' '
#define  ZERO				  '\0'

struct mk_f_list
{
        char *name;
        char *ft_modif;
        struct file_info *info;
};

/* Estructura de lista de archivos y directorios */
struct f_list {
	long len;
	char path[MAX_PATH+1];			/* Ruta de acceso               */
	char size[MAX_SIZE+1];			/* Tama�o del archivo           */
	char ft_modif[MAX_TIME+1];	  /* Fecha y hora de modificacion */
};

/* Sort string characters by shell method
 * Code taken from Kernighan & Ritchie book 
 */
struct f_list *shell (struct f_list *b, int n)
{
	int				gap, i, j;
	struct f_list  temp;

        for (gap = n/2; gap > 0; gap /= 2)
        {
          for (i = gap; i <= n; i ++)
          {         
			for (j = i-gap;
					j >= 0 && ((strcmp(b[j].path, b[j+gap].path))>0);
					j -= gap)
			{
				temp			  = b[j];
				b[j]			  = b[j+gap];
				b[j+gap]		 = temp;
			}
          }
        }
	return (struct f_list *)b;
}


/* Si encuentra un ' ' en la cadena lo reemplaza por su valor 
   en hexadecimal (%20). Autor: Eduardo Silva */
char *check_string(char *str)
{
	int cnt=0;
	char *s, *f;
	char *final_buffer=0;

	if (str==NULL)
		return str;

	for(s=str;*s!='\0';++s)
		if(*s==' ') cnt++;

	if(cnt==0)
		return mk_string_dup(str);

	final_buffer=mk_mem_malloc(strlen(str)+1+(cnt*3));

	for(f=final_buffer,s=str;*s!='\0';++s) {
		if (*s==' ') {
			*f++='%'; *f++='2'; *f++='0';
		} else {
			*f++=*s;
		}
	}

	*f='\0';
	return final_buffer;
}

/* Recortar la cadena si excede el ancho de MAX_LEN_STR */
char *cut_string(char *str)
{
	int i, k, j, len;
	char *s;

	if ((len = strlen(str)) > MAX_LEN_STR) {
		s=mk_mem_malloc(MAX_LEN_STR);
		k = MAX_LEN_STR/2 - 2;
		for (i=0; i<k; i++)
			s[i] = str[i];

		s[i++] = '.';
		s[i++] = '.';
		s[i++] = '.';
		j		= i;
		k		= len - k;

		for (i=k; str[i]; i++, j++)
			s[j] = str[i];

		s[j] = ZERO;
		return (char *) s;
	}

	return mk_string_dup(str);
}

struct mk_iov *mk_dirhtml_iov(struct mk_f_list *list, int len)
{
        char *chunked_line;
        int i;
        int iov_len;

        struct mk_iov *data_iov;

        iov_len = (len*5) + 2 + 1;
        data_iov = mk_iov_create(iov_len);
        
        /* tricky update, offset +1 to keep chunked data */
        data_iov->iov_idx = 1;

        //        mk_iov_add_entry(data_iov, header, 12, MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);

        for(i=0; i<len; i++)
        {
          //mk_iov_add_entry(data_iov, PRE1, len_pre1, MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);
          //mk_iov_add_entry(data_iov, list[i].name, strlen(list[i].name), MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);
          //mk_iov_add_entry(data_iov, PRE2, len_pre2, MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);
          //mk_iov_add_entry(data_iov, list[i].name, strlen(list[i].name), MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);
          //mk_iov_add_entry(data_iov, PRE3, len_pre3, MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);
        }

        //mk_iov_add_entry(data_iov, footer, 14, MK_IOV_NONE, MK_IOV_NOT_FREE_BUF);

        /* Total length to send */
        chunked_line = (char *) mk_header_chunked_line(data_iov->total_len);
        data_iov->io[0].iov_base = chunked_line; 
        data_iov->io[0].iov_len = strlen(chunked_line);

        return (struct mk_iov *) data_iov;
}

void mk_dirhtml_add_element(struct mk_f_list *list, char *file,
                            char *full_path, unsigned long *count)
{
        list[*count].name = file;
        list[*count].info = (struct file_info *) mk_file_get_info(full_path);

        *count = *count + 1;
}

int mk_dirhtml_create_list(DIR *dir, struct mk_f_list *file_list,
                           char *path, unsigned long *list_len, int offset)
{
	unsigned long len;
        char *full_path;
        struct dirent *ent;

	/* Before to send the information, we need to build
         * the list of entries, this really sucks because the user
         * always will want to have the information sorted, why we don't 
         * add some spec to the HTTP protocol in order to send the information
         * in a generic way and let the client choose how to show it
         * as they does browsing a FTP server ???, we can save bandweight,
         * let the cool firefox developers create different templates and
         * we are going to have a more happy end users.
         *
         * that kind of ideas comes when you are in an airport just waiting :)
         */
	while((ent = readdir(dir)) != NULL)
	{
                if(strcmp((char *) ent->d_name, "." )  == 0) continue;
                if(strcmp((char *) ent->d_name, ".." ) == 0) continue;

                /* Look just for files and dirs */
                if(ent->d_type!=DT_REG && ent->d_type!=DT_DIR)
                {
                        continue;
                }

		m_build_buffer(&full_path, &len, "%s%s", path, ent->d_name);
		
		if(!ent->d_name)
                {
			puts("mk_dirhtml :: buffer error");
		}

		mk_dirhtml_add_element(file_list, ent->d_name, full_path, list_len);

                if (!file_list)
                {
                        closedir(dir);
			return -1;
		}
 	}
        
        return 0;
}

/* Read dirhtml config and themes */
int mk_dirhtml_conf()
{
        int ret = 0;
        unsigned long len;
        char *themes_path;
        
        m_build_buffer(&themes_path, &len, "%s/dir_themes/", config->serverconf);
        ret = mk_dirhtml_read_config(themes_path);

        /* This function will load the default theme 
         * setted in dirhtml_conf struct 
         */
        mk_dirhtml_theme_load();

        return ret;
}

/* 
 * Read the main configuration file for dirhtml: dirhtml.conf, 
 * it will alloc the dirhtml_conf struct
*/
int mk_dirhtml_read_config(char *path)
{
        unsigned long len;
        char *default_file;
        char buffer[255];
        FILE *fileconf;
        char *variable, *value, *last;

        m_build_buffer(&default_file, &len, "%sdirhtml.conf", path);

        if(!(fileconf = fopen(default_file, "r")))
        {
          puts("Error: Cannot open dirhtml conf file");
          return -1;
        }

        /* alloc dirhtml config struct */
        dirhtml_conf = mk_mem_malloc(sizeof(struct dirhtml_config));

        while(fgets(buffer, 255, fileconf))
        {
                 len = strlen(buffer);
                 if(buffer[len-1] == '\n') {
                   buffer[--len] = 0;
                   if(len && buffer[len-1] == '\r')
                     buffer[--len] = 0;
                 }
        
                 if(!buffer[0] || buffer[0] == '#')
                          continue;
            
                 variable   = strtok_r(buffer, "\"\t ", &last);
                 value     = strtok_r(NULL, "\"\t ", &last);

                 if (!variable || !value) continue;

                 /* Server Name */
                 if(strcasecmp(variable,"Theme")==0)
                 {
                         dirhtml_conf->theme = mk_string_dup(value);
                         m_build_buffer(&dirhtml_conf->theme_path, &len, 
                                        "%s%s/", path, dirhtml_conf->theme);
                 }
        }
        fclose(fileconf);
        return 0;
}

int mk_dirhtml_theme_load()
{
        /* List of Values */
        char *lov_header[] = MK_DIRHTML_TPL_HEADER;
        char *lov_entry[] = MK_DIRHTML_TPL_ENTRY;
        char *lov_footer[] = MK_DIRHTML_TPL_FOOTER;

        /* Data */
	char *header, *entry, *footer;

        /* Templates */
        struct dirhtml_template *tpl_header, *tpl_entry, *tpl_footer;

        /* Load theme files */
        header = mk_dirhtml_load_file(MK_DIRHTML_FILE_HEADER);
        entry = mk_dirhtml_load_file(MK_DIRHTML_FILE_ENTRY);
        footer = mk_dirhtml_load_file(MK_DIRHTML_FILE_FOOTER);

        if(!header || !entry || !footer)
        {
                mk_mem_free(header);
                mk_mem_free(entry);
                mk_mem_free(footer);
                return -1;
        }

        /* Parse themes */
        tpl_header = mk_dirhtml_theme_parse(header, lov_header);
        tpl_entry = mk_dirhtml_theme_parse(entry, lov_entry);
        tpl_footer = mk_dirhtml_theme_parse(footer, lov_footer);

#ifdef DEBUG_THEME
        /* Debug data */
        mk_dirhtml_theme_debug(tpl_header, lov_header);
        mk_dirhtml_theme_debug(tpl_entry, lov_entry);
        mk_dirhtml_theme_debug(tpl_footer, lov_footer);
#endif
        return 0;
}

#ifdef DEBUG_THEME
int mk_dirhtml_theme_debug(struct dirhtml_template *st_tpl, char *tpl[])
{
        int i;

        printf("\nSTARTING DEBUG");
        for(i=0; st_tpl[i].len!=EOF; i++){
                if(!st_tpl[i].buf)
                {
                        printf("\n%i %s", i, tpl[st_tpl[i].len]);
                }
                else{
                        printf("\n%i) %s", i, st_tpl[i].buf);
                }

                fflush(stdout);
        }
        return 0;
}
#endif

/* Search which tag exists first in content :
 * ex: %_html_title_%
 */
int mk_dirhtml_theme_match_tag(char *content, char *tpl[])
{
        int i, len, match;
        
        for(i=0; tpl[i]; i++){
                len = strlen(tpl[i]);
                match = _mk_string_search(content, tpl[i], -1);
                if(match>=0){
                        return i;
                }
        }

        return -1;
              
}


struct dirhtml_template *mk_dirhtml_theme_parse(char *content, char *tpl[])
{
        int i=0, arr_len, cont_len;
        int pos, last=0; /* 0=search init, 1=search end */
        int n_tags=0, idx=0, tpl_idx=0;
        struct dirhtml_template *st_tpl;

        if((cont_len = strlen(content))<=0){
                return NULL;
        };
        
        arr_len = mk_string_array_count(tpl);

        /* Alloc memory for the typical case where exist n_tags, 
         * no repetitive tags
         */
        st_tpl = mk_mem_malloc_z(sizeof(struct dirhtml_template)*((arr_len*2)+1));

        /* Parsing content */
        for(i=0; i<cont_len; i++)
        {
                pos = _mk_string_search(content+last,
                                                MK_DIRHTML_TAG_INIT, -1);

                /*
                printf("\npos: %i, last: %i", pos, last);
                fflush(stdout);
                */

                if(pos<0){
                        break;
                }


                tpl_idx = mk_dirhtml_theme_match_tag(content+pos, tpl);
                if(tpl_idx>=0){
                        if(pos>0){
                                st_tpl[idx].buf = mk_string_copy_substr(content, 
                                                                        last, pos+last);
                                st_tpl[idx].len = strlen(st_tpl[idx].buf);
                                idx++;
                        }
                        last += pos+strlen(tpl[i]);

                        /* This means that a value need to be replaced */
                        st_tpl[idx].buf = NULL;
                        st_tpl[idx].len = tpl_idx;

                        idx++;
                        n_tags++;
                }
        }

/*
        printf("\nidx: %i, last: %i, cont_len: %i", idx, last, cont_len);
        fflush(stdout);
*/
        if(last<cont_len){
                st_tpl[idx].buf = mk_string_copy_substr(content, last, cont_len);
                st_tpl[idx].len = strlen(st_tpl[idx].buf);
        }

        idx++;
        st_tpl[idx].buf = NULL;
        st_tpl[idx].len = EOF;

        return (struct dirhtml_template *) st_tpl;
}

char *mk_dirhtml_load_file(char *filename)
{
        char *tmp=0, *data=0;
        unsigned long len;

        m_build_buffer(&tmp, &len, "%s%s",
                       dirhtml_conf->theme_path, filename);

        if(!tmp)
        {
                return NULL;
        }

        data = mk_file_to_buffer(tmp);
        mk_mem_free(tmp);

        if(!data)
        {
                return NULL;
        }

        return (char *) data;
}

int mk_dirhtml_init(struct client_request *cr, struct request *sr)
{
        DIR *dir;
        int ret, i;
        unsigned long len;
        char *chunked_line;

	/* file info */
	unsigned long list_len=0;
        struct mk_f_list *file_list;
        struct mk_iov *html_list;
 
        if(!(dir = opendir(sr->real_path)))
        {
                return -1;
        }

	/* handle dir entries */
        printf("\nmk_dirhtml_init :: %s", sr->real_path);
        fflush(stdout);

        file_list = mk_mem_malloc(
                                  sizeof(struct mk_f_list)*
                                  MK_DIRHTML_BUFFER_LIMIT);

        ret = mk_dirhtml_create_list(dir, file_list, sr->real_path, &list_len, 0);

	// FIXME: Need sort file list	
        sr->headers->transfer_encoding = -1;
	sr->headers->status = M_HTTP_OK;
	sr->headers->cgi = SH_CGI;
        sr->headers->breakline = MK_HEADER_BREAKLINE;

	m_build_buffer(&sr->headers->content_type, &len, "text/html");

	// FIXME: Check this counter
	//hd->pconnections_left = config->max_keep_alive_request - cr->counter_connections;

        /*
	if(sr->protocol==HTTP_PROTOCOL_11)
	{
		sr->headers->transfer_encoding = MK_HEADER_TE_TYPE_CHUNKED;
	}
	*/

	/* Sending headers */
	mk_header_send(cr->socket, cr, sr, sr->log);

        html_list = mk_dirhtml_iov(file_list, list_len);

        //mk_iov_add_entry(html_list, chunked_line, strlen(chunked_line), MK_IOV_NONE, MK_IOV_FREE_BUF);
        mk_iov_send(cr->socket, html_list);
        mk_socket_set_cork_flag(cr->socket, TCP_CORK_OFF);

        for (i=0; i<list_len; i++)
	{
                printf("\n dir -> %s", file_list[i].name);
                fflush(stdout);

                //char* c_str = check_string(file_list[i].path);
		//char* x_str = cut_string(file_list[i].path);
		//char *c_str = file_list[i].path;
		//char *x_str = file_list[i].path;

		//data = m_build_buffer_from_buffer(data,
		/* printf( 	" %s  %s   <A HREF=\"%s\">%s</A>\n",*/
		//	file_list[i].ft_modif, file_list[i].size,
		//	c_str, x_str);

		//mk_mem_free(c_str);
		//mk_mem_free(x_str);
        }

	//mk_mem_free(file_list);
	//mk_mem_free(data);

        closedir(dir);
	return -1;
}



/* Send information of current directory on HTML format
   Modified : 2007/01/21
   -> Add struct client_request support

   Modified : 2002/10/22 
   -> Chunked Transfer Encoding support added to HTTP/1.1

  FIXME: REWRITE THIS SECTION >:)
*/

