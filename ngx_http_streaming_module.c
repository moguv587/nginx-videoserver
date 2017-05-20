/*******************************************************************************
 ngx_http_h264_streaming_module.c

 mod_h264_streaming - An Nginx module for streaming Quicktime/MPEG4 files.

 Copyright (C) 2008-2009 CodeShop B.V.

 Licensing
 The Streaming Module is licened under a Creative Commons License. It
 allows you to use, modify and redistribute the module, but only for
 *noncommercial* purposes. For corporate use, please apply for a
 commercial license.

 Creative Commons License:
 http://creativecommons.org/licenses/by-nc-sa/3.0/

 Commercial License for H264 Streaming Module:
 http://h264.code-shop.com/trac/wiki/Mod-H264-Streaming-License-Version2

 Commercial License for Smooth Streaming Module:
 http://smoothstreaming.code-shop.com/trac/wiki/Mod-Smooth-Streaming-License
******************************************************************************/ 

#include <nginx.h>
#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_http.h>
#include "mp4_io.h"
#include "mp4_process.c"
#include "moov.h"
#include "output_bucket.h"
#ifdef BUILDING_H264_STREAMING
#include "output_mp4.h"
#define X_MOD_STREAMING_KEY X_MOD_H264_STREAMING_KEY
#define X_MOD_STREAMING_VERSION X_MOD_H264_STREAMING_VERSION
#endif
#ifdef BUILDING_SMOOTH_STREAMING
#include "ism_reader.h"
#include "output_ismv.h"
#define X_MOD_STREAMING_KEY X_MOD_SMOOTH_STREAMING_KEY
#define X_MOD_STREAMING_VERSION X_MOD_SMOOTH_STREAMING_VERSION
#endif
#ifdef BUILDING_FLV_STREAMING
#include "output_flv.h"
#endif

#if 0
/* Mod-H264-Streaming configuration

server {
  listen 82;
  server_name  localhost;

  location ~ \.mp4$ {
    root /var/www;
    mp4;
  }
}

*/

/* Mod-Smooth-Streaming configuration

server {
  listen 82;
  server_name localhost;

  rewrite ^(.*/)?(.*)\.([is])sm/[Mm]anifest$ $1$2.$3sm/$2.ismc last;
  rewrite ^(.*/)?(.*)\.([is])sm/QualityLevels\(([0-9]+)\)/Fragments\((.*)=([0-9]+)\)(.*)$ $1$2.$3sm/$2.ism?bitrate=$4&$5=$6 last;

  location ~ \.ism$ {
    root /var/www;
    ism;
  }
}
*/
#endif

static char* ngx_streaming(ngx_conf_t* cf, ngx_command_t* cmd, void* conf);

static ngx_command_t ngx_streaming_commands[] =
{
  {
#ifdef BUILDING_H264_STREAMING
    ngx_string("mp4"),
#endif
#ifdef BUILDING_SMOOTH_STREAMING
    ngx_string("ism"),
#endif
    NGX_HTTP_LOC_CONF|NGX_CONF_NOARGS,
    ngx_streaming,
    0,
    0,
    NULL
  },
  ngx_null_command
};

static ngx_http_module_t ngx_streaming_module_ctx =
{
  NULL,                          /* preconfiguration */
  NULL,                          /* postconfiguration */

  NULL,                          /* create main configuration */
  NULL,                          /* init main configuration */

  NULL,                          /* create server configuration */
  NULL,                          /* merge server configuration */

  NULL,                           /* create location configuration */
  NULL                            /* merge location configuration */
};

#ifdef BUILDING_H264_STREAMING
ngx_module_t ngx_http_h264_streaming_module =
#endif
#ifdef BUILDING_SMOOTH_STREAMING
ngx_module_t ngx_http_smooth_streaming_module =
#endif
{
  NGX_MODULE_V1,
  &ngx_streaming_module_ctx,     /* module context */
  ngx_streaming_commands,        /* module directives */
  NGX_HTTP_MODULE,               /* module type */
  NULL,                          /* init master */
  NULL,                          /* init module */
  NULL,                          /* init process */
  NULL,                          /* init thread */
  NULL,                          /* exit thread */
  NULL,                          /* exit process */
  NULL,                          /* exit master */
  NGX_MODULE_V1_PADDING
};

static ngx_int_t ngx_streaming_handler(ngx_http_request_t *r)
{
  u_char                      *last;
  size_t                      root;
  ngx_int_t                   rc;
  ngx_uint_t                  level;
  ngx_str_t                   path;
  char                        filename[256];
  ngx_log_t                   *log;
  ngx_open_file_info_t        of;
  ngx_http_core_loc_conf_t    *clcf;

  if (!(r->method & (NGX_HTTP_GET|NGX_HTTP_HEAD)))
  {
    return NGX_HTTP_NOT_ALLOWED;
  }

  if (r->uri.data[r->uri.len - 1] == '/')
  {
    return NGX_DECLINED;
  }

  /* TODO: Win32
  if (r->zero_in_uri)
  {
    return NGX_DECLINED;
  }*/

  rc = ngx_http_discard_request_body(r);

  if(rc != NGX_OK)
  {
    return rc;
  }

  mp4_split_options_t* options = mp4_split_options_init();
  if(r->args.len && !mp4_split_options_set(options, (const char*)r->args.data, r->args.len))
  {
    mp4_split_options_exit(options);
    return NGX_DECLINED;
  }

  last = ngx_http_map_uri_to_path(r, &path, &root, 0);
  if (last == NULL)
  {
    return NGX_HTTP_INTERNAL_SERVER_ERROR;
  }

  log = r->connection->log;

  path.len = last - path.data;

  ngx_cpystrn((u_char*)filename, path.data, sizeof(filename) / sizeof(char) - 1);
  filename[sizeof(filename) / sizeof(char) - 1] = '\0';

#ifdef BUILDING_SMOOTH_STREAMING
  // if it is a fragment request then we read the server manifest file
  // and based on the bitrate and track type we set the filename and track id
  if(ends_with(filename, ".ism"))
  {
    if(options->output_format != OUTPUT_FORMAT_TS)
    {
      ism_t* ism = ism_init(filename);

      if(ism == NULL)
      {
        return NGX_HTTP_NOT_FOUND;
      }

      const char* src;
      if(!ism_get_source(ism, options->fragment_bitrate, options->fragment_type,
                         &src, &options->fragment_track_id))
      {
        return NGX_HTTP_NOT_FOUND;
      }

      char* dir_end = strrchr(filename, '/');
      dir_end = dir_end == NULL ? filename : (dir_end + 1);
      strcpy(dir_end, src);

      ism_exit(ism);
    }
  }
#endif

  path.len = strlen(filename);
  path.data = ngx_pnalloc(r->pool, path.len + 1);
  memcpy(path.data, filename, path.len);
  // ngx_open_and_stat_file in ngx_open_cached_file expects the name to be zero-terminated.
  path.data[path.len] = '\0';

  ngx_log_debug1(NGX_LOG_DEBUG_HTTP, log, 0,
                 "http mp4 filename: \"%V\"", &path);

  clcf = ngx_http_get_module_loc_conf(r, ngx_http_core_module);

  ngx_memzero(&of, sizeof(ngx_open_file_info_t));

#if nginx_version >= 8018
  of.read_ahead = clcf->read_ahead;
#endif
  of.directio = clcf->directio;
  of.valid = clcf->open_file_cache_valid;
  of.min_uses = clcf->open_file_cache_min_uses;
  of.errors = clcf->open_file_cache_errors;
  of.events = clcf->open_file_cache_events;

  if(ngx_open_cached_file(clcf->open_file_cache, &path, &of, r->pool) != NGX_OK)
  {
    switch (of.err)
    {
    case 0:
        return NGX_HTTP_INTERNAL_SERVER_ERROR;
    case NGX_ENOENT:
    case NGX_ENOTDIR:
    case NGX_ENAMETOOLONG:
        level = NGX_LOG_ERR;
        rc = NGX_HTTP_NOT_FOUND;
        break;
    case NGX_EACCES:
        level = NGX_LOG_ERR;
        rc = NGX_HTTP_FORBIDDEN;
        break;
    default:
        level = NGX_LOG_CRIT;
        rc = NGX_HTTP_INTERNAL_SERVER_ERROR;
        break;
    }

    if (rc != NGX_HTTP_NOT_FOUND || clcf->log_not_found)
    {
#if nginx_version >= 8018
      ngx_log_error(level, log, of.err,
                    "%s \"%s\" failed", of.failed, path.data);
#else
      ngx_log_error(level, log, of.err,
                    ngx_open_file_n "%s \"%s\" failed", path.data);
#endif
    }

    return rc;
  }

  if (!of.is_file)
  {
    if(ngx_close_file(of.fd) == NGX_FILE_ERROR)
    {
      ngx_log_error(NGX_LOG_ALERT, log, ngx_errno,
                    ngx_close_file_n " \"%s\" failed", path.data);
    }
    return NGX_DECLINED;
  }

  // ??
  r->root_tested = !r->error_page;

  {
    struct bucket_t* buckets = 0;
    int verbose = 0;
    int http_status =
      mp4_process((const char*)path.data, of.size, verbose, &buckets, options);

    mp4_split_options_exit(options);

    if(http_status != 200)
    {
      if(buckets)
      {
        buckets_exit(buckets);
      }

      return http_status;
    }

    r->headers_out.content_type.len = sizeof("video/mp4") - 1;
    r->headers_out.content_type.data = (u_char*)"video/mp4";

    ngx_chain_t* out = 0;
    ngx_chain_t** chain = &out;
    uint64_t content_length = 0;

    {
      struct bucket_t* bucket = buckets;
      if(bucket)
      {
        do
        {
          ngx_buf_t* b = ngx_pcalloc(r->pool, sizeof(ngx_buf_t));
          *chain = ngx_pcalloc(r->pool, sizeof(ngx_chain_t));
          if(b == NULL || *chain == NULL)
          {
            return NGX_HTTP_INTERNAL_SERVER_ERROR;
          }

          switch(bucket->type_)
          {
          case BUCKET_TYPE_MEMORY:
            b->pos = ngx_pcalloc(r->pool, bucket->size_);
            if(b->pos == NULL)
            {
              return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            b->last = b->pos + bucket->size_;
            b->memory = 1;
            memcpy(b->pos, bucket->buf_, bucket->size_);
            break;
          case BUCKET_TYPE_FILE:
            b->file = ngx_pcalloc(r->pool, sizeof(ngx_file_t));
            if(b->file == NULL)
            {
              return NGX_HTTP_INTERNAL_SERVER_ERROR;
            }
            b->file->fd = of.fd;
            b->file->name = path;
            b->file->log = log;
            b->file_pos = bucket->offset_;
            b->file_last = b->file_pos + bucket->size_;
            b->in_file = b->file_last ? 1: 0;
            break;
          }

          b->last_buf = bucket->next_ == buckets ? 1 : 0;
          b->last_in_chain = bucket->next_ == buckets ? 1 : 0;

          (*chain)->buf = b;
          (*chain)->next = NULL;
          chain = &(*chain)->next;

          content_length += bucket->size_;
          bucket = bucket->next_;
        } while(bucket != buckets);
        buckets_exit(buckets);
      }
    }

    log->action = "sending mp4 to client";

    r->headers_out.status = NGX_HTTP_OK;
    r->headers_out.content_length_n = content_length;
    r->headers_out.last_modified_time = of.mtime;

    // Nginx properly handles range requests, even when we're sending chunks
    // from both memory (the metadata) and part file (the movie data).
    r->allow_ranges = 1;

//      if(ngx_http_set_content_type(r) != NGX_OK)
//      {
//        return NGX_HTTP_INTERNAL_SERVER_ERROR;
//      }

    ngx_table_elt_t* h = ngx_list_push(&r->headers_out.headers);
    if(h == NULL)
    {
      return NGX_HTTP_INTERNAL_SERVER_ERROR;
    }
    h->hash = 1;

    h->key.len = sizeof(X_MOD_STREAMING_KEY) - 1;
    h->key.data = (u_char*)X_MOD_STREAMING_KEY;
    h->value.len = sizeof(X_MOD_STREAMING_VERSION) - 1;
    h->value.data = (u_char*)X_MOD_STREAMING_VERSION;

    rc = ngx_http_send_header(r);

    if (rc == NGX_ERROR || rc > NGX_OK || r->header_only) {
        return rc;
    }

    return ngx_http_output_filter(r, out);
  }
}

static char* ngx_streaming(ngx_conf_t* cf, ngx_command_t* cmd, void* conf)
{
  ngx_http_core_loc_conf_t* clcf =
    ngx_http_conf_get_module_loc_conf(cf, ngx_http_core_module);

  clcf->handler = ngx_streaming_handler;

  return NGX_CONF_OK;
}

// End Of File

