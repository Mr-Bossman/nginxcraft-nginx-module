// SPDX-License-Identifier: GPL-2.0+
/*
 * parse_minecraft.c
 *
 * This module is a simple example of how to parse the server name from a
 * stream session. It is based on the ngx_stream_ssl_preread_module.c module
 * that is included with NGINX. This module is intended to be used as a
 * reference for developers who are looking to create their own stream
 * modules.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#include <stdbool.h>

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

#include "ngx_stream_nginxcraft_module.h"

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

typedef struct VarInt VarInt;
struct VarInt {
    int32_t value;
    size_t length;
    bool valid;
};

typedef struct mc_string mc_string;
struct mc_string {
    u_char* data;
    size_t data_length;
    bool valid;
};

typedef struct minecraft_packet minecraft_packet;
struct minecraft_packet {
    VarInt length;
    VarInt packetId;
    u_char* data;
    size_t data_length;
    bool valid;
};

static int parse_handshake(minecraft_packet* packet, mc_string* serv_Address);
static int parse_packet(u_char* buffer, size_t length, minecraft_packet* packet);
static mc_string read_mc_string(u_char* buffer, size_t length);
static VarInt readVarInt(u_char* buffer, size_t length);

ngx_int_t
submodule_nginxcraft_add_variables(ngx_conf_t *cf)
{
    (void)cf;
    return NGX_OK;
}

ngx_int_t ngx_stream_nginxcraft_parse(
    ngx_stream_nginxcraft_ctx_t *ctx, ngx_buf_t *buf)
{
    u_char *p = buf->pos;
    size_t len = p - buf->last;;
    minecraft_packet packet;
    mc_string serv_Address;
    int ret;

    ret = parse_packet(p, len, &packet);
    if (ret != 0) {
        return NGX_ERROR;
    }

    ret = parse_handshake(&packet, &serv_Address);
    if (ret != 0) {
        return NGX_ERROR;
    }

    ctx->host.data = ngx_pnalloc(ctx->pool, serv_Address.data_length + 1);
    if (ctx->host.data == NULL) {
        return NGX_ERROR;
    }
    ctx->host.data[serv_Address.data_length] = '\0';
    ctx->host.len = serv_Address.data_length;
    (void)ngx_cpymem(ctx->host.data, serv_Address.data, serv_Address.data_length);

    ngx_log_debug(NGX_LOG_DEBUG_STREAM, ctx->log, 0, "stream nginxcraft: %s",  ctx->host.data);
// sed -n 's/.*stream nginxcraft: //gp'  /usr/local/nginx/logs/debug.log |  xxd -r -p | hexdump -C
    return NGX_OK;
}


static VarInt readVarInt(u_char* buffer, size_t length)
{
    VarInt ret = {0, 0, true};
    size_t ind = 0;

    while (ind < 32 && ind < length) {
        ret.value |= (buffer[ind] & SEGMENT_BITS) << (ind*7);

        if ((buffer[ind] & CONTINUE_BIT) == 0) break;

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

static mc_string read_mc_string(u_char* buffer, size_t length) {
    mc_string ret = {NULL, 0, true};

    VarInt stringLength = readVarInt(buffer, length);
    if (!stringLength.valid) {
        ret.valid = false;
        return ret;
    }
    if (stringLength.length > length) {
        ret.valid = false;
        return ret;
    }
    ret.data = buffer + stringLength.length;
    ret.data_length = stringLength.value;
    return ret;
}

static int parse_packet(u_char* buffer, size_t length, minecraft_packet* packet)
{
    size_t Length_ID_sz = 0;
    size_t Packet_ID_sz = 0;
    packet->length = readVarInt(buffer, length);
    Length_ID_sz = packet->length.length;
    if (!packet->length.valid) {
        packet->valid = false;
        return -1;
    }
    if(Length_ID_sz > length) {
        packet->valid = false;
        return -1;
    }
    length -= Length_ID_sz;
    packet->packetId = readVarInt(buffer + Length_ID_sz, length);
    Packet_ID_sz = packet->packetId.length;
    if (!packet->packetId.valid) {
        packet->valid = false;
        return -1;
    }
    if(Packet_ID_sz > length) {
        packet->valid = false;
        return -1;
    }
    length -= Packet_ID_sz;
    packet->data = buffer + Length_ID_sz + Packet_ID_sz;
    packet->data_length = length;
    return 0;
}

static int parse_handshake(minecraft_packet* packet, mc_string* serv_Address)
{
    u_char *data = packet->data;
    size_t data_length = packet->data_length;
    size_t protocolVersion_sz = 0;
    size_t serv_Address_sz = 0;
    VarInt protocolVersion;

    if (packet->packetId.value != 0) {
        return -1;
    }

    protocolVersion = readVarInt(data, data_length);
    protocolVersion_sz = protocolVersion.length;
    if (!protocolVersion.valid) {
        return -1;
    }
    if (protocolVersion_sz > data_length) {
        return -1;
    }
    data_length -= protocolVersion_sz;
    *serv_Address = read_mc_string(data + protocolVersion_sz, data_length);
    serv_Address_sz = serv_Address->data_length;
    if (!serv_Address->valid) {
        return -1;
    }
    if (serv_Address_sz > 255) {
        serv_Address->valid = false;
        return -1;
    }
    return 0;
}
