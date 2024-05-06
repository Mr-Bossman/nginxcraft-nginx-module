// SPDX-License-Identifier: GPL-2.0+
/*
 * parse_minecraft.c
 *
 * Functions for parsing Minecraft packets and returning the
 * values to NGINX.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#include <stdbool.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_nginxcraft_module.h"
#include "minecraft_funcs.h"

#define ngx_snprint_int(buf, len, val) \
    ngx_str_snprintf(buf, len, "%d", val)

#define ngx_snprint_uint(buf, len, val) \
    ngx_str_snprintf(buf, len, "%u", val)


static ngx_int_t ngx_str_snprintf(ngx_str_t* ret, size_t sz, const char *fmt, ...);

static ngx_int_t minecraft_port_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t minecraft_version_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);

static ngx_stream_variable_t  nginxcraft_vars[] = {

    { ngx_string("minecraft_port"), NULL,
      minecraft_port_variable, 0, 0, 0 },

    { ngx_string("minecraft_version"), NULL,
      minecraft_version_variable, 0, 0, 0 },

      ngx_stream_null_variable
};

ngx_int_t
submodule_nginxcraft_add_variables(ngx_conf_t *cf)
{
    ngx_stream_variable_t   *var, *v;

    for (v = nginxcraft_vars; v->name.len; v++) {

        var = ngx_stream_add_variable(cf, &v->name, v->flags);

        if (var == NULL) {
            return NGX_ERROR;
        }

        var->get_handler = v->get_handler;
        var->data = v->data;
    }

    return NGX_OK;
}

static ngx_int_t minecraft_port_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_stream_nginxcraft_ctx_t *ctx;
    nginxcraft_var              *vars;
    (void)data;

    ctx = ngx_stream_get_module_ctx(s, ngx_stream_nginxcraft_module);
    vars = (nginxcraft_var *)ctx->variables;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ctx == NULL || vars == NULL) {
        v->len = 0;
        v->data = NULL;
        return NGX_OK;
    }

    v->len = vars->minecraft_port.len;
    v->data = vars->minecraft_port.data;

    return NGX_OK;
}

static ngx_int_t minecraft_version_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data)
{
    ngx_stream_nginxcraft_ctx_t *ctx;
    nginxcraft_var              *vars;
    (void)data;

    ctx = ngx_stream_get_module_ctx(s, ngx_stream_nginxcraft_module);
    vars = (nginxcraft_var *)ctx->variables;

    v->valid = 1;
    v->no_cacheable = 0;
    v->not_found = 0;

    if (ctx == NULL || vars == NULL) {
        v->len = 0;
        v->data = NULL;
        return NGX_OK;
    }

    v->len = vars->minecraft_version.len;
    v->data = vars->minecraft_version.data;

    return NGX_OK;
}

ngx_int_t ngx_stream_nginxcraft_parse(
    ngx_stream_nginxcraft_ctx_t *ctx, ngx_buf_t *buf)
{
    u_char              *p = buf->pos;
    size_t               len = p - buf->last;;
    minecraft_packet     packet;
    minecraft_handshake  handshake;
    nginxcraft_var      *vars;
    int                  ret;

    ret = parse_packet(p, len, &packet);

    if (ret != NGX_OK) {
        return NGX_DECLINED;
    }

    ret = parse_handshake(&packet, &handshake);

    if (ret != NGX_OK) {
        return NGX_DECLINED;
    }

    ctx->variables = ngx_pnalloc(ctx->pool, sizeof(nginxcraft_var));

    if (ctx->variables == NULL) {
        return NGX_ERROR;
    }

    vars = (nginxcraft_var *)ctx->variables;

    ctx->host.data = ngx_pnalloc(ctx->pool, handshake.serv_Address.data_length);

    if (ctx->host.data == NULL) {
        return NGX_ERROR;
    }

    ret = mc_str2ngx_str(&ctx->host, handshake.serv_Address.data_length, handshake.serv_Address);

    if (ret != NGX_OK) {
        return NGX_ERROR;
    }

    vars->minecraft_port.data = ngx_pnalloc(ctx->pool, 6);

    if (vars->minecraft_port.data == NULL) {
        return NGX_ERROR;
    }

    vars->minecraft_version.data = ngx_pnalloc(ctx->pool, 11);

    if (vars->minecraft_version.data == NULL) {
        return NGX_ERROR;
    }

    ngx_snprint_uint(&vars->minecraft_port, 6, handshake.serv_Port);
    ngx_snprint_int(&vars->minecraft_version, 11, handshake.protocolVersion);

    ngx_log_debug(NGX_LOG_DEBUG_STREAM, ctx->log, 0, "nginxcraft parse: %s",  ctx->host.data);

    return NGX_OK;
}


static ngx_int_t ngx_str_snprintf(ngx_str_t* ret, size_t sz, const char *fmt, ...)
{
    //u_char   *p;
    va_list   args;

    if (ret->data == NULL) {
        return NGX_ERROR;
    }

    va_start(args, fmt);
    //p = ngx_vsnprintf(ret->data, sz, fmt, args);
    (void)vsnprintf((char*)ret->data, sz, fmt, args);
    ret->len = strnlen((char*)ret->data, sz);
    va_end(args);

    //ret->len = p - ret->data;

    return NGX_OK;
}
