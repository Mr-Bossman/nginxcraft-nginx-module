// SPDX-License-Identifier: GPL-2.0+
/*
 * ngx_stream_nginxcraft_module.h
 *
 * This module is a simple example of how to parse the server name from a
 * stream session. It is based on the ngx_stream_ssl_preread_module.c module
 * that is included with NGINX. This module is intended to be used as a
 * reference for developers who are looking to create their own stream
 * modules.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#ifndef NGX_STREAM_NGINXCRAFT_MODULE_H
#define NGX_STREAM_NGINXCRAFT_MODULE_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

typedef struct {
    ngx_flag_t                   enabled;
    ngx_stream_complex_value_t   text;
} ngx_stream_nginxcraft_srv_conf_t;


typedef struct {
    ngx_str_t        host;
    ngx_log_t       *log;
    ngx_pool_t      *pool;
    void            *variables;
    ngx_chain_t     *out;
} ngx_stream_nginxcraft_ctx_t;

extern ngx_module_t ngx_stream_nginxcraft_module;

ngx_int_t ngx_stream_nginxcraft_parse(ngx_stream_nginxcraft_ctx_t *ctx, ngx_buf_t *buf);
ngx_int_t submodule_nginxcraft_add_variables(ngx_conf_t *cf);

#endif /* NGX_STREAM_NGINXCRAFT_MODULE_H */
