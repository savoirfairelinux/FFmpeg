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

#include "imxvpuapi_decode.h"

typedef struct imxvpuapiDecH264Context {
        AVClass *class;
        imxvpuapiDecContext imxvpuapi;
} imxvpuapiDecH264Context;

static int imxvpuapi_h264_dec_init(AVCodecContext *avctx)
{
        imxvpuapiDecH264Context *h264_ctx = avctx->priv_data;
        imxvpuapiDecContext *ctx = &h264_ctx->imxvpuapi;

        ctx->compression_format = IMX_VPU_API_COMPRESSION_FORMAT_H264;

        return ff_imxvpuapi_dec_init(avctx, &h264_ctx->imxvpuapi);
}

static int imxvpuapi_h264_dec_frame(AVCodecContext *avctx, void *data,
                                    int *got_frame, AVPacket *avpkt)
{
        imxvpuapiDecH264Context *h264_ctx = avctx->priv_data;

        return ff_imxvpuapi_dec_frame(avctx, &h264_ctx->imxvpuapi, data,
                                      got_frame, avpkt);
}

static int imxvpuapi_h264_dec_close(AVCodecContext *avctx)
{
        imxvpuapiDecH264Context *h264_ctx = avctx->priv_data;

        return ff_imxvpuapi_dec_close(avctx, &h264_ctx->imxvpuapi);
}

static const AVClass imxvpuapi_dec_h264_class = {
        .class_name = "h264_imxvpuapi decoder",
        .item_name  = av_default_item_name,
        .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_h264_imxvpuapi_decoder = {
        .name           = "h264_imxvpuapi",
        .long_name      = NULL_IF_CONFIG_SMALL("H.264 imxvpuapi decoder"),
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = AV_CODEC_ID_H264,
        .priv_data_size = sizeof(imxvpuapiDecH264Context),
        .init           = imxvpuapi_h264_dec_init,
        .decode         = imxvpuapi_h264_dec_frame,
        .close          = imxvpuapi_h264_dec_close,
        .priv_class     = &imxvpuapi_dec_h264_class,
        .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_HARDWARE,
        .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
        .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_YUV420P,
                                                        AV_PIX_FMT_NONE },
        .bsfs           = "h264_mp4toannexb",
        .wrapper_name   = "imxvpuapi",
};
