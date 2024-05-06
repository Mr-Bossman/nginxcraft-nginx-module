
/*
 * Copyright (C) Roman Arutyunyan
 * Copyright (C) Nginx, Inc.
 * Original file: src/stream/ngx_stream_return_module.c
 */


#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_nginxcraft_module.h"
#include "ngx_stream_nginxcraft_return_module.h"
#include "minecraft_funcs.h"

static void ngx_stream_return_handler(ngx_stream_session_t *s);
static void ngx_stream_return_write_handler(ngx_event_t *ev);
static ngx_int_t ngx_stream_nginxcraft_create_disconnect_packet(ngx_connection_t *c,
    const ngx_str_t *value, ngx_str_t *minecraft_str);

static void
ngx_stream_return_handler(ngx_stream_session_t *s)
{
    ngx_str_t                      text;
    ngx_str_t                      minecraft_str;
    ngx_buf_t                     *b;
    ngx_connection_t              *c;
    ngx_stream_nginxcraft_ctx_t       *ctx;
    ngx_stream_nginxcraft_srv_conf_t  *nscf;

    c = s->connection;

    c->log->action = "returning text";

    nscf = ngx_stream_get_module_srv_conf(s, ngx_stream_nginxcraft_module);

    if (ngx_stream_complex_value(s, &nscf->text, &text) != NGX_OK) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_log_debug1(NGX_LOG_DEBUG_STREAM, c->log, 0,
                   "stream return text: \"%V\"", &text);

    if (text.len == 0) {
        ngx_stream_finalize_session(s, NGX_STREAM_OK);
        return;
    }

    ctx = ngx_pcalloc(c->pool, sizeof(ngx_stream_nginxcraft_ctx_t));
    if (ctx == NULL) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_stream_set_ctx(s, ctx, ngx_stream_nginxcraft_module);

    b = ngx_calloc_buf(c->pool);
    if (b == NULL) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    if (ngx_stream_nginxcraft_create_disconnect_packet(c, &text, &minecraft_str) != NGX_OK) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    b->memory = 1;
    b->pos = minecraft_str.data;
    b->last = minecraft_str.data + minecraft_str.len;
    b->last_buf = 1;

    ctx->out = ngx_alloc_chain_link(c->pool);
    if (ctx->out == NULL) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    ctx->out->buf = b;
    ctx->out->next = NULL;

    c->write->handler = ngx_stream_return_write_handler;

    ngx_stream_return_write_handler(c->write);
}


static void
ngx_stream_return_write_handler(ngx_event_t *ev)
{
    ngx_connection_t         *c;
    ngx_stream_session_t     *s;
    ngx_stream_nginxcraft_ctx_t  *ctx;

    c = ev->data;
    s = c->data;

    if (ev->timedout) {
        ngx_connection_error(c, NGX_ETIMEDOUT, "connection timed out");
        ngx_stream_finalize_session(s, NGX_STREAM_OK);
        return;
    }

    ctx = ngx_stream_get_module_ctx(s, ngx_stream_nginxcraft_module);

    if (ngx_stream_top_filter(s, ctx->out, 1) == NGX_ERROR) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    ctx->out = NULL;

    if (!c->buffered) {
        ngx_log_debug0(NGX_LOG_DEBUG_STREAM, c->log, 0,
                       "stream return done sending");
        ngx_stream_finalize_session(s, NGX_STREAM_OK);
        return;
    }

    if (ngx_handle_write_event(ev, 0) != NGX_OK) {
        ngx_stream_finalize_session(s, NGX_STREAM_INTERNAL_SERVER_ERROR);
        return;
    }

    ngx_add_timer(ev, 5000);
}

char *
ngx_stream_nginxcraft_return(ngx_conf_t *cf, ngx_command_t *cmd, void *conf)
{
    ngx_stream_nginxcraft_srv_conf_t    *nscf = conf;

    ngx_str_t                           *value;
    ngx_stream_core_srv_conf_t          *cscf;
    ngx_stream_compile_complex_value_t   ccv;

    if (nscf->text.value.data) {
        return "is duplicate";
    }

    value = cf->args->elts;

    ngx_memzero(&ccv, sizeof(ngx_stream_compile_complex_value_t));

    ccv.cf = cf;
    ccv.value = &value[1];
    ccv.complex_value = &nscf->text;

    if (ngx_stream_compile_complex_value(&ccv) != NGX_OK) {
        return NGX_CONF_ERROR;
    }

    cscf = ngx_stream_conf_get_module_srv_conf(cf, ngx_stream_core_module);

    cscf->handler = ngx_stream_return_handler;

    return NGX_CONF_OK;
}

static ngx_int_t
ngx_stream_nginxcraft_create_disconnect_packet(ngx_connection_t *c, const ngx_str_t *value,
    ngx_str_t *minecraft_str)
{

    minecraft_str->len = get_disconnect_packet_size(value->len);
    minecraft_str->data = ngx_pcalloc(c->pool, minecraft_str->len);

    if (minecraft_str->data == NULL) {
        return NGX_ERROR;
    }

    create_disconnect_packet(minecraft_str->data, value->data, value->len);

    return NGX_OK;
}
