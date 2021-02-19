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

#ifndef AVCODEC_IMXVPUAPI_DECODE_H
#define AVCODEC_IMXVPUAPI_DECODE_H

#include "imxvpuapi2/imxvpuapi2.h"
#include "avcodec.h"
#include "internal.h"
#include <stdbool.h>

typedef struct imxvpuapiDecContext {
        AVCodecContext *avctx;

        ImxVpuApiDecoder *decoder;

        ImxDmaBufferAllocator *allocator;

        ImxDmaBuffer *stream_buffer;

        ImxVpuApiDecGlobalInfo const *dec_global_info;

        ImxVpuApiDecStreamInfo stream_info;

        ImxDmaBuffer **fb_pool_dmabuffers;
        size_t num_fb_pool_framebuffers;

        ImxDmaBuffer *output_dmabuffer;

        ImxVpuApiCompressionFormat compression_format;
        int uv_height;
        int uv_width;
        int uv_stride;
        bool first_frame;
} imxvpuapiDecContext;

int ff_imxvpuapi_dec_init(AVCodecContext *avctx, imxvpuapiDecContext *ctx);

int ff_imxvpuapi_dec_frame(AVCodecContext *avctx, imxvpuapiDecContext *ctx, void
                           *data, int *got_frame, AVPacket *avpkt);

int ff_imxvpuapi_dec_close(AVCodecContext *avctx, imxvpuapiDecContext *ctx);

#endif /* AVCODEC_IMXVPUAPI_DECODE_H */
