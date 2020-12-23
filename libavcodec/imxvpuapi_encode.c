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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */



#include "imxvpuapi_encode.h"

int ff_imxvpuapi_enc_init(AVCodecContext *avctx)
{
        imxvpuapiEncContext *ctx = avctx->priv_data;

        ctx->enc_global_info = imx_vpu_api_enc_get_global_info();
        return 0;
}

int ff_imxvpuapi_enc_frame(AVCodecContext *avctx, AVPacket *pkt,
                           const AVFrame *frame, int *got_packet)
{
        return 0;
}

int ff_imxvpuapi_enc_close(AVCodecContext *avctx)
{
        return 0;
}
