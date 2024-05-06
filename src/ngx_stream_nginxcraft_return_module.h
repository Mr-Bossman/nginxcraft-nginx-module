// SPDX-License-Identifier: GPL-2.0+
/*
 * ngx_stream_nginxcraft_return_module.h
 *
 * Copyright (C) 2024-2025 Jesse Taube <Mr.Bossman075@gmail.com>
 */

#ifndef NGX_STREAM_NGINXCRAFT_RETURN_MODULE_H
#define NGX_STREAM_NGINXCRAFT_RETURN_MODULE_H

#include <ngx_config.h>
#include <ngx_core.h>
#include <ngx_stream.h>

char *ngx_stream_nginxcraft_return(ngx_conf_t *cf, ngx_command_t *cmd, void *conf);

#endif /* NGX_STREAM_NGINXCRAFT_RETURN_MODULE_H */
