// SPDX-License-Identifier: GPL-2.0+
/*
 * submodules.c
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

#include "ngx_stream_parse_server_name_module.h"

ngx_int_t
submodule_parse_server_name_add_variables(ngx_conf_t *cf)
{
    (void)cf;
    return NGX_OK;
}

static u_char *get_http_host(u_char *data, size_t len)
{
    while (len > 0) {
        len--;
        if(*data++ == '\n') {
            break;
        }
    }
    if (len > 6 && ngx_strncmp(data, (u_char*)"Host: ", 6) == 0) {
        data += 6;
        return data;
    }
    return NULL;
}

ngx_int_t ngx_stream_parse_server_name_parse(
    ngx_stream_parse_server_name_ctx_t *ctx, ngx_buf_t *buf)
{
    u_char *p, *last, *url;
    size_t len, i = 0;
    p = buf->pos;
    last = buf->last;
    len = p - last;

    u_char data[256];
    memset(data, 0, 256);
    url = get_http_host(p, len);
    if (url == NULL) {
        return NGX_AGAIN;
    }
    while (url < last && url[i] != '\r' && i < 256) {
        data[i] = url[i];
        i++;
    }

    ctx->host.data = ngx_pnalloc(ctx->pool, i);
    if (ctx->host.data == NULL) {
        return NGX_ERROR;
    }
    (void)ngx_cpymem(ctx->host.data, data, i);
    ctx->host.len = i;

    (void)ngx_hex_dump(data, p, ngx_min(len*2, 127));
    ngx_log_debug(NGX_LOG_DEBUG_STREAM, ctx->log, 0, "stream parse_server_name: %s", data);
// sed -n 's/.*stream parse_server_name: //gp'  /usr/local/nginx/logs/debug.log |  xxd -r -p | hexdump -C
    return NGX_OK;
}
