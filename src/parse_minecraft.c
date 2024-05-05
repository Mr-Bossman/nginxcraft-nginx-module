// SPDX-License-Identifier: GPL-2.0+
/*
 * parse_minecraft.c
 *
 * Functions for parsing Minecraft packets.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#include <stdbool.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_nginxcraft_module.h"

#define ngx_snprint_int(buf, len, val) \
    ngx_str_snprintf(buf, len, "%d", val)

#define ngx_snprint_uint(buf, len, val) \
    ngx_str_snprintf(buf, len, "%u", val)

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

typedef struct nginxcraft_var nginxcraft_var;
struct nginxcraft_var {
    ngx_str_t minecraft_port;
    ngx_str_t minecraft_version;
};

typedef struct VarInt VarInt;
struct VarInt {
    int32_t      value;
    size_t       length;
    bool         valid;
};

typedef struct mc_string mc_string;
struct mc_string {
    const u_char      *data;
    size_t             data_length;
    bool               valid;
};

typedef struct minecraft_packet minecraft_packet;
struct minecraft_packet {
    VarInt           length;
    VarInt           packetId;
    const u_char    *data;
    size_t           data_length;
    bool             valid;
};

typedef struct minecraft_handshake minecraft_handshake;
struct minecraft_handshake {
    int32_t      protocolVersion;
    mc_string    serv_Address;
    uint16_t     serv_Port;
    int32_t      nextState;
    bool         valid;
};

static ngx_int_t ngx_str_snprintf(ngx_str_t* ret, size_t sz, const char *fmt, ...);

static ngx_int_t mc_str2ngx_str(ngx_str_t* ret, size_t sz, const mc_string mc_str);
static ngx_int_t minecraft_port_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t minecraft_version_variable(
    ngx_stream_session_t *s, ngx_stream_variable_value_t *v, uintptr_t data);
static ngx_int_t parse_handshake(const minecraft_packet* packet,
    minecraft_handshake* handshake);
static ngx_int_t parse_packet(const u_char* buffer, size_t length, minecraft_packet* packet);
static mc_string read_mc_string(const u_char* buffer, size_t length);
static VarInt readVarInt(const u_char* buffer, size_t length);

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


static ngx_int_t mc_str2ngx_str(ngx_str_t* ret, size_t sz, const mc_string mc_str)
{
    if(sz < mc_str.data_length || ret->data == NULL) {
        return NGX_ERROR;
    }

    ret->len = mc_str.data_length;
    (void)ngx_cpymem(ret->data, mc_str.data, mc_str.data_length);

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

static VarInt readVarInt(const u_char* buffer, size_t length)
{
    VarInt   ret = {0, 0, true};
    size_t   ind = 0;

    while (true) {
        if (ind >= length) {
            ret.valid = false;
            ret.value = -1;
            // Not enough data
            return ret;
        }

        ret.value |= (buffer[ind] & SEGMENT_BITS) << (ind*7);

        if ((buffer[ind] & CONTINUE_BIT) == 0)
            break;

        ind++;

        if ((ind*7) >= 32) {
            ret.valid = false;
            ret.value = -1;
            // Value is too large to be a varint
            return ret;
        }
    }

    ret.length = ind + 1;

    return ret;
}

static mc_string read_mc_string(const u_char* buffer, size_t length) {
    mc_string    ret = {NULL, 0, true};
    VarInt       stringLength = readVarInt(buffer, length);

    if (!stringLength.valid) {
        ret.valid = false;
        return ret;
    }

    ret.data = buffer + stringLength.length;
    ret.data_length = stringLength.value;

    return ret;
}

static ngx_int_t parse_packet(const u_char* buffer, size_t length, minecraft_packet* packet)
{
    size_t   Length_ID_sz = 0;
    size_t   Packet_ID_sz = 0;

    packet->length = readVarInt(buffer, length);
    Length_ID_sz = packet->length.length;

    if (!packet->length.valid) {
        packet->valid = false;
        return NGX_ERROR;
    }

    buffer += Length_ID_sz;
    length -= Length_ID_sz;
    packet->packetId = readVarInt(buffer, length);
    Packet_ID_sz = packet->packetId.length;

    if (!packet->packetId.valid) {
        packet->valid = false;
        return NGX_ERROR;
    }

    packet->data = buffer + Packet_ID_sz;
    packet->data_length = length - Packet_ID_sz;

    return NGX_OK;
}

static ngx_int_t
parse_handshake(const minecraft_packet* packet, minecraft_handshake* handshake)
{
    const u_char    *data = packet->data;
    size_t           data_length = packet->data_length;
    size_t           protocolVersion_sz = 0;
    size_t           serv_Address_sz = 0;
    //size_t           nextState_sz = 0;
    VarInt           protocolVersion, nextState;

    handshake->valid = false;
    handshake->serv_Address.valid = false;

    if (packet->packetId.value != 0) {
        return NGX_ERROR;
    }

    protocolVersion = readVarInt(data, data_length);
    protocolVersion_sz = protocolVersion.length;

    if (!protocolVersion.valid) {
        return NGX_ERROR;
    }

    handshake->protocolVersion = protocolVersion.value;

    data += protocolVersion_sz;
    data_length -= protocolVersion_sz;
    handshake->serv_Address = read_mc_string(data, data_length);
    serv_Address_sz = handshake->serv_Address.data - data;
    serv_Address_sz += handshake->serv_Address.data_length;

    if (!handshake->serv_Address.valid) {
        return NGX_ERROR;
    }

    if (serv_Address_sz > 255) {
        return NGX_ERROR;
    }

    data += serv_Address_sz;
    data_length -= serv_Address_sz;

    if(2 > data_length) {
        return NGX_ERROR;
    }

    handshake->serv_Port = be16toh(*(uint16_t*)data);
    data += 2;
    data_length -= 2;
    nextState = readVarInt(data, data_length);
    //nextState_sz = nextState.length;

    if (!nextState.valid) {
        return NGX_ERROR;
    }

    /*
    // sends multiple packets
    if (nextState_sz < data_length) {
        return NGX_ERROR;
    }
    */

    handshake->nextState = nextState.value;
    handshake->serv_Address.valid = true;
    handshake->valid = true;

    return NGX_OK;
}
