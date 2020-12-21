/*
 * Copyright (c) 2020 Savoir-faire Linux, Inc
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "imxvpuapi_encode.h"
#include "internal.h"

typedef struct imxvpuapiEncH264Context {
} imxvpuapiEncH264Context;

static const AVClass imxvpuapi_enc_h264_class = {
};

AVCodec ff_h264_imxvpuapi_encoder = {
        .name           = "h264_imxvpuapi",
        .long_name      = NULL_IF_CONFIG_SMALL("H.264 imxvpuapi encoder"),
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = AV_CODEC_ID_H264,
        .priv_data_size = sizeof(imxvpuapiEncH264Context),
        .init           = ff_imxvpuapi_enc_init,
        .encode2        = ff_imxvpuapi_enc_frame,
        .close          = ff_imxvpuapi_enc_close,
        .priv_class     = &imxvpuapi_enc_h264_class,
        .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_HARDWARE,
        .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
        .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_YUYV422,
                                                        AV_PIX_FMT_NONE },
        .wrapper_name   = "imxvpuapi",
};
