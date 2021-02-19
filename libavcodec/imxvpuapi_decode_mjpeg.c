/*
 * Copyright (c) 2021 Savoir-faire Linux, Inc
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

typedef struct imxvpuapiDecMjepgContext {
        AVClass *class;
        imxvpuapiDecContext imxvpuapi;
} imxvpuapiDecMjpegContext;

static int imxvpuapi_mjpeg_dec_init(AVCodecContext *avctx)
{
        imxvpuapiDecMjpegContext *mjpeg_ctx = avctx->priv_data;
        imxvpuapiDecContext *ctx = &mjpeg_ctx->imxvpuapi;

        ctx->compression_format = IMX_VPU_API_COMPRESSION_FORMAT_JPEG;

        return ff_imxvpuapi_dec_init(avctx, &mjpeg_ctx->imxvpuapi);
}

static int imxvpuapi_mjpeg_dec_frame(AVCodecContext *avctx, void *data,
                                    int *got_frame, AVPacket *avpkt)
{
        imxvpuapiDecMjpegContext *mjpeg_ctx = avctx->priv_data;

        return ff_imxvpuapi_dec_frame(avctx, &mjpeg_ctx->imxvpuapi, data,
                                      got_frame, avpkt);
}

static int imxvpuapi_mjpeg_dec_close(AVCodecContext *avctx)
{
        imxvpuapiDecMjpegContext *mjpeg_ctx = avctx->priv_data;

        return ff_imxvpuapi_dec_close(avctx, &mjpeg_ctx->imxvpuapi);
}

static const AVClass imxvpuapi_dec_mjpeg_class = {
        .class_name = "mjpeg_imxvpuapi decoder",
        .item_name  = av_default_item_name,
        .version    = LIBAVUTIL_VERSION_INT,
};

AVCodec ff_mjpeg_imxvpuapi_decoder = {
        .name           = "mjpeg_imxvpuapi",
        .long_name      = NULL_IF_CONFIG_SMALL("MJPEG imxvpuapi decoder"),
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = AV_CODEC_ID_MJPEG,
        .priv_data_size = sizeof(imxvpuapiDecMjpegContext),
        .init           = imxvpuapi_mjpeg_dec_init,
        .decode         = imxvpuapi_mjpeg_dec_frame,
        .close          = imxvpuapi_mjpeg_dec_close,
        .priv_class     = &imxvpuapi_dec_mjpeg_class,
        .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_HARDWARE,
        .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
        .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_YUV420P,
                                                        AV_PIX_FMT_NONE },
        .wrapper_name   = "imxvpuapi",
};
