// SPDX-License-Identifier: GPL-2.0+
/*
 * minecraft_funcs.h
 *
 * Helper funcrtions for parsing Minecraft packets.
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#ifndef MINECRAFT_FUNCS_H
#define MINECRAFT_FUNCS_H

#include <stdbool.h>

#include <ngx_config.h>
#include <ngx_core.h>

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

ngx_int_t parse_handshake(const minecraft_packet* packet, minecraft_handshake* handshake);
ngx_int_t parse_packet(const u_char* buffer, size_t length, minecraft_packet* packet);
mc_string read_mc_string(const u_char* buffer, size_t length);
VarInt readVarInt(const u_char* buffer, size_t length);

ngx_int_t mc_str2ngx_str(ngx_str_t* ret, size_t sz, const mc_string mc_str);

#endif /* MINECRAFT_FUNCS_H */
