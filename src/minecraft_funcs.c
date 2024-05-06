// SPDX-License-Identifier: GPL-2.0+
/*
 * minecraft_funcs.c
 *
 * Helper funcrtions for parsing Minecraft packets.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

 #include <stdbool.h>

#include <ngx_config.h>
#include <ngx_core.h>

#include "minecraft_funcs.h"

#define SEGMENT_BITS 0x7F
#define CONTINUE_BIT 0x80

ngx_int_t mc_str2ngx_str(ngx_str_t* ret, size_t sz, const mc_string mc_str)
{
    if(sz < mc_str.data_length || ret->data == NULL) {
        return NGX_ERROR;
    }

    ret->len = mc_str.data_length;
    (void)ngx_cpymem(ret->data, mc_str.data, mc_str.data_length);

    return NGX_OK;
}

VarInt readVarInt(const u_char* buffer, size_t length)
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

mc_string read_mc_string(const u_char* buffer, size_t length) {
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

ngx_int_t parse_packet(const u_char* buffer, size_t length, minecraft_packet* packet)
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

ngx_int_t parse_handshake(const minecraft_packet* packet, minecraft_handshake* handshake)
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
