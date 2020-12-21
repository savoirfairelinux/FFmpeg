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

typedef struct imxvpuapiEncH264Context {
        AVClass *class;
        imxvpuapiEncContext imxvpuapi;
} imxvpuapiEncH264Context;

static const AVClass imxvpuapi_enc_h264_class = {
        .class_name = "h264_imxvpuapi encoder",
        .item_name  = av_default_item_name,
        .version    = LIBAVUTIL_VERSION_INT,
};

static int imxvpuapi_h264_enc_init(AVCodecContext *avctx)
{
        imxvpuapiEncH264Context *h264_ctx = avctx->priv_data;
        imxvpuapiEncContext *ctx = &h264_ctx->imxvpuapi;

        ctx->compression_format = IMX_VPU_API_COMPRESSION_FORMAT_H264;

        return ff_imxvpuapi_enc_init(avctx, &h264_ctx->imxvpuapi);
}

static int imxvpuapi_h264_enc_frame(AVCodecContext *avctx, AVPacket *pkt,
                                    const AVFrame *frame, int *got_packet)
{
        imxvpuapiEncH264Context *h264_ctx = avctx->priv_data;

        return ff_imxvpuapi_enc_frame(avctx, &h264_ctx->imxvpuapi, pkt, frame,
                                      got_packet);
}

static int imxvpuapi_h264_enc_close(AVCodecContext *avctx)
{
        imxvpuapiEncH264Context *h264_ctx = avctx->priv_data;

        return ff_imxvpuapi_enc_close(avctx, &h264_ctx->imxvpuapi);
}

AVCodec ff_h264_imxvpuapi_encoder = {
        .name           = "h264_imxvpuapi",
        .long_name      = NULL_IF_CONFIG_SMALL("H.264 imxvpuapi encoder"),
        .type           = AVMEDIA_TYPE_VIDEO,
        .id             = AV_CODEC_ID_H264,
        .priv_data_size = sizeof(imxvpuapiEncH264Context),
        .init           = imxvpuapi_h264_enc_init,
        .encode2        = imxvpuapi_h264_enc_frame,
        .close          = imxvpuapi_h264_enc_close,
        .priv_class     = &imxvpuapi_enc_h264_class,
        .capabilities   = AV_CODEC_CAP_DELAY | AV_CODEC_CAP_HARDWARE,
        .caps_internal  = FF_CODEC_CAP_INIT_CLEANUP,
        .pix_fmts       = (const enum AVPixelFormat[]){ AV_PIX_FMT_YUV420P,
                                                        AV_PIX_FMT_YUV422P,
                                                        AV_PIX_FMT_YUV444P,
                                                        AV_PIX_FMT_YUV411P,
                                                        AV_PIX_FMT_P010,
                                                        AV_PIX_FMT_NONE },
        .wrapper_name   = "imxvpuapi",
};
