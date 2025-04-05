// SPDX-License-Identifier: GPL-2.0+
/*
 * ngx_stream_nginxcraft_module.c
 *
 * This module is a simple example of how to parse the server name from a
 * stream session. It is based on the ngx_stream_ssl_preread_module.c module
 * that is included with NGINX. This module is intended to be used as a
 * reference for developers who are looking to create their own stream
 * modules.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_nginxcraft_module.h"
#include "ngx_stream_nginxcraft_return_module.h"

static void *ngx_stream_nginxcraft_create_srv_conf(ngx_conf_t *cf);
static ngx_int_t ngx_stream_nginxcraft_servername(ngx_stream_session_t *s,
    ngx_str_t *servername);
static ngx_int_t ngx_stream_nginxcraft_handler(ngx_stream_session_t *s);
static ngx_int_t ngx_stream_nginxcraft_init(ngx_conf_t *cf);
static ngx_int_t ngx_stream_nginxcraft_add_variables(ngx_conf_t *cf);
static ngx_int_t ngx_stream_servername_host_variable(ngx_stream_session_t *s,
    ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_command_t ngx_stream_nginxcraft_commands[] = {

    { ngx_string("nginxcraft"),
      NGX_STREAM_MAIN_CONF|NGX_STREAM_SRV_CONF|NGX_CONF_FLAG,
      ngx_conf_set_flag_slot,
      NGX_STREAM_SRV_CONF_OFFSET,
      offsetof(ngx_stream_nginxcraft_srv_conf_t, enabled),
      NULL },

    { ngx_string("nginxcraft_return"),
      NGX_STREAM_SRV_CONF|NGX_CONF_TAKE1,
      ngx_stream_nginxcraft_return,
      NGX_STREAM_SRV_CONF_OFFSET,
      0,
      NULL },

      ngx_null_command
};


static ngx_stream_module_t ngx_stream_nginxcraft_module_ctx = {
    ngx_stream_nginxcraft_add_variables,     /* preconfiguration */
    ngx_stream_nginxcraft_init,              /* postconfiguration */

    NULL,                                    /* create main configuration */
    NULL,                                    /* init main configuration */

    ngx_stream_nginxcraft_create_srv_conf,   /* create server configuration */
    NULL                                     /* merge server configuration */
};


ngx_module_t ngx_stream_nginxcraft_module = {
    NGX_MODULE_V1,
    &ngx_stream_nginxcraft_module_ctx,     /* module context */
    ngx_stream_nginxcraft_commands,        /* module directives */
    NGX_STREAM_MODULE,                     /* module type */
    NULL,                                  /* init master */
    NULL,                                  /* init module */
    NULL,                                  /* init process */
    NULL,                                  /* init thread */
    NULL,                                  /* exit thread */
    NULL,                                  /* exit process */
    NULL,                                  /* exit master */
    NGX_MODULE_V1_PADDING
};

static ngx_stream_variable_t ngx_stream_nginxcraft_vars[] = {

    { ngx_string("minecraft_server"), NULL,
      ngx_stream_servername_host_variable, 0, 0, 0 },

      ngx_stream_null_variable
};


static void *
ngx_stream_nginxcraft_create_srv_conf(ngx_conf_t *cf)
{
    ngx_stream_nginxcraft_srv_conf_t    *conf;

    conf = ngx_pcalloc(cf->pool, sizeof(ngx_stream_nginxcraft_srv_conf_t));

    if (conf == NULL) {
        return NULL;
    }

    conf->enabled = NGX_CONF_UNSET;

    return conf;
}

static ngx_int_t
ngx_stream_nginxcraft_handler(ngx_stream_session_t *s)
{
    ngx_int_t                    rc;
    ngx_connection_t            *c;
    ngx_stream_nginxcraft_ctx_t *ctx;

    c = s->connection;

    ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0, "nginxcraft handler");

    if (c->buffer == NULL) {
        return NGX_AGAIN;
    }

    ctx = ngx_stream_get_module_ctx(s, ngx_stream_nginxcraft_module);

    if (ctx == NULL) {
        ctx = ngx_pcalloc(c->pool, sizeof(ngx_stream_nginxcraft_ctx_t));

        if (ctx == NULL) {
            return NGX_ERROR;
        }

        ctx->pool = c->pool;
        ctx->log = c->log;
        ngx_stream_set_ctx(s, ctx, ngx_stream_nginxcraft_module);
    }

    rc = ngx_stream_nginxcraft_parse(ctx, c->buffer);

    if (rc == NGX_OK) {
        return ngx_stream_nginxcraft_servername(s, &ctx->host);
    }

    if (rc == NGX_DECLINED) {
        return NGX_OK;
    }

    return rc;
}

static ngx_int_t
ngx_stream_nginxcraft_servername(ngx_stream_session_t *s,
    ngx_str_t *servername)
{
    ngx_int_t                            rc;
    ngx_str_t                            host;
    ngx_connection_t                    *c;
    ngx_stream_core_srv_conf_t          *cscf;
    ngx_stream_nginxcraft_srv_conf_t    *nscf;

    c = s->connection;

    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0,
                   "nginxcraft server name: \"%V\"", servername);

    if (servername->len == 0) {
        return NGX_OK;
    }

    host = *servername;

    rc = ngx_stream_validate_host(&host, c->pool, 1);

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    if (rc == NGX_DECLINED) {
        return NGX_OK;
    }

    rc = ngx_stream_find_virtual_server(s, &host, &cscf);

    if (rc == NGX_ERROR) {
        return NGX_ERROR;
    }

    if (rc == NGX_DECLINED) {
        return NGX_OK;
    }

    if(cscf->ctx->srv_conf == NULL) {
        return NGX_OK;
    }

    nscf = ngx_stream_get_module_srv_conf(cscf->ctx, ngx_stream_nginxcraft_module);

    if (nscf->enabled != 1) {
        return NGX_OK;
    }

    s->srv_conf = cscf->ctx->srv_conf;

    ngx_set_connection_log(c, cscf->error_log);

    return NGX_OK;
}

static ngx_int_t
ngx_stream_servername_host_variable(ngx_stream_session_t *s,
    ngx_variable_value_t *v, uintptr_t data)
{
    ngx_stream_nginxcraft_ctx_t *ctx;

    ctx = ngx_stream_get_module_ctx(s, ngx_stream_nginxcraft_module);

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ctx == NULL || ctx->host.len == 0) {
        v->len = 0;
        v->data = NULL;
        return NGX_OK;
    }

    v->len = ctx->host.len;
    v->data = ctx->host.data;

    return NGX_OK;
}

static ngx_int_t
ngx_stream_nginxcraft_add_variables(ngx_conf_t *cf)
{
    ngx_stream_variable_t   *var, *v;

    for (v = ngx_stream_nginxcraft_vars; v->name.len; v++) {

        var = ngx_stream_add_variable(cf, &v->name, v->flags);

        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return submodule_nginxcraft_add_variables(cf);
}

static ngx_int_t
ngx_stream_nginxcraft_init(ngx_conf_t *cf)
{
    ngx_stream_handler_pt       *h;
    ngx_stream_core_main_conf_t *cmcf;

    cmcf = ngx_stream_conf_get_module_main_conf(cf, ngx_stream_core_module);

    h = ngx_array_push(&cmcf->phases[NGX_STREAM_PREREAD_PHASE].handlers);

    if (h == NULL) {
        return NGX_ERROR;
    }

    *h = ngx_stream_nginxcraft_handler;

    return NGX_OK;
}
