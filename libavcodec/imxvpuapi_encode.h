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

#ifndef AVCODEC_IMXVPUAPI_ENCODE_H
#define AVCODEC_IMXVPUAPI_ENCODE_H

#include "imxvpuapi2/imxvpuapi2.h"
#include "avcodec.h"
#include "internal.h"

typedef struct imxvpuapiEncContext {
        AVCodecContext *avctx;

        ImxVpuApiEncoder *encoder;

        ImxDmaBufferAllocator *allocator;

        ImxDmaBuffer *stream_buffer;

        ImxVpuApiEncGlobalInfo const *enc_global_info;

        ImxVpuApiEncStreamInfo stream_info;

        ImxDmaBuffer **fb_pool_dmabuffers;
        size_t num_fb_pool_framebuffers;

        ImxDmaBuffer *input_dmabuffer;

        ImxVpuApiEncOpenParams open_params;

        void *encoded_frame_buffer;
        size_t encoded_frame_buffer_size;

        ImxVpuApiCompressionFormat compression_format;

        ImxVpuApiColorFormat color_format;
} imxvpuapiEncContext;

int ff_imxvpuapi_enc_init(AVCodecContext *avctx, imxvpuapiEncContext *ctx);

int ff_imxvpuapi_enc_frame(AVCodecContext *avctx, imxvpuapiEncContext *ctx,
                           AVPacket *pkt, const AVFrame *frame, int *got_packet);

int ff_imxvpuapi_enc_close(AVCodecContext *avctx, imxvpuapiEncContext *ctx);

#endif /* AVCODEC_IMXVPUAPI_ENCODE_H */
