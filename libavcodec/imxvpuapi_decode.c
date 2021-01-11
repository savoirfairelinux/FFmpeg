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

#include "imxvpuapi_decode.h"

static int allocate_output_framebuffer(AVCodecContext *avctx,
                                       imxvpuapiDecContext *ctx)
{
        int err = 0;

        if (ctx->output_dmabuffer != NULL)
        {
                imx_dma_buffer_deallocate(ctx->output_dmabuffer);
                ctx->output_dmabuffer = NULL;
        }

        ctx->output_dmabuffer = imx_dma_buffer_allocate(ctx->allocator,
                                                        ctx->stream_info.min_output_framebuffer_size,
                                                        ctx->stream_info.output_framebuffer_alignment,
                                                        &err);
        if (ctx->output_dmabuffer == NULL)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not allocate DMA buffer for FB pool framebuffer: %s (%d)\n",
                       strerror(err), err);
                return 1;
        }
        return 0;
}

static int allocate_and_add_fb_pool_framebuffers(AVCodecContext *avctx,
                                                 imxvpuapiDecContext *ctx,
                                                 size_t
                                                 num_fb_pool_framebuffers_to_add,
                                                 ImxVpuApiRawFrame decoded_frame)
{
        ImxDmaBuffer **new_fb_pool_dmabuffers_array;
        size_t old_num_fb_pool_framebuffers;
        size_t new_num_fb_pool_framebuffers;
        ImxVpuApiDecReturnCodes dec_ret;
        void **fb_contexts = NULL;
        int err = 0;
        size_t i;

        if (num_fb_pool_framebuffers_to_add == 0)
                return 0;

        old_num_fb_pool_framebuffers = ctx->num_fb_pool_framebuffers;
        new_num_fb_pool_framebuffers = old_num_fb_pool_framebuffers +
                num_fb_pool_framebuffers_to_add;

        new_fb_pool_dmabuffers_array = realloc(ctx->fb_pool_dmabuffers,
                                               new_num_fb_pool_framebuffers *
                                               sizeof(ImxDmaBuffer *));
        if(!new_fb_pool_dmabuffers_array)
                return 1;

        memset(new_fb_pool_dmabuffers_array + old_num_fb_pool_framebuffers, 0,
               num_fb_pool_framebuffers_to_add * sizeof(ImxDmaBuffer *));

        ctx->fb_pool_dmabuffers = new_fb_pool_dmabuffers_array;
        ctx->num_fb_pool_framebuffers = new_num_fb_pool_framebuffers;

        fb_contexts = malloc(num_fb_pool_framebuffers_to_add * sizeof(void *));
        if(!fb_contexts)
                return 1;

        for (i = old_num_fb_pool_framebuffers; i <
             ctx->num_fb_pool_framebuffers; ++i)
        {
                ctx->fb_pool_dmabuffers[i] =
                        imx_dma_buffer_allocate(ctx->allocator,
                                                ctx->stream_info.min_fb_pool_framebuffer_size,
                                                ctx->stream_info.fb_pool_framebuffer_alignment,
                                                &err);
                if (ctx->fb_pool_dmabuffers[i] == NULL)
                {
                        av_log(avctx, AV_LOG_ERROR, "Could not allocate DMA buffer for FB pool framebuffer: %s (%d)\n",
                               strerror(err), err);
                        free(fb_contexts);
                        return 1;
                }
                fb_contexts[i - old_num_fb_pool_framebuffers] =
                        (void *)((int*)decoded_frame.fb_context + i);
        }

        dec_ret = imx_vpu_api_dec_add_framebuffers_to_pool(ctx->decoder,
                                                           ctx->fb_pool_dmabuffers
                                                           +
                                                           old_num_fb_pool_framebuffers,
                                                           fb_contexts,
                                                           num_fb_pool_framebuffers_to_add);
        if (dec_ret != IMX_VPU_API_DEC_RETURN_CODE_OK)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not add framebuffers to VPU pool: %s\n",
                       imx_vpu_api_dec_return_code_string(dec_ret));
                free(fb_contexts);
                return 1;
        }

        free(fb_contexts);
        return 0;
}


static void deallocate_framebuffers(imxvpuapiDecContext *ctx)
{
        size_t i;

        for (i = 0; i < ctx->num_fb_pool_framebuffers; ++i)
        {
                ImxDmaBuffer *dmabuffer = ctx->fb_pool_dmabuffers[i];
                if (dmabuffer != NULL)
                        imx_dma_buffer_deallocate(dmabuffer);
        }
        free(ctx->fb_pool_dmabuffers);

        ctx->num_fb_pool_framebuffers = 0;
        ctx->fb_pool_dmabuffers = NULL;
}

static int push_encoded_input_frame(AVCodecContext *avctx, imxvpuapiDecContext
                                    *ctx, AVPacket *avpkt)
{
        ImxVpuApiDecReturnCodes dec_ret;
        ImxVpuApiEncodedFrame encoded_frame;

        memset(&encoded_frame, 0, sizeof(encoded_frame));

        encoded_frame.data = avpkt->data;
        encoded_frame.data_size = avpkt->size;

        dec_ret = imx_vpu_api_dec_push_encoded_frame(ctx->decoder,
                                                     &encoded_frame);
        if (dec_ret == IMX_VPU_API_DEC_RETURN_CODE_INVALID_CALL)
        {
                imx_vpu_api_dec_flush(ctx->decoder);
                dec_ret = imx_vpu_api_dec_push_encoded_frame(ctx->decoder,
                                                             &encoded_frame);
        }
        if (dec_ret != IMX_VPU_API_DEC_RETURN_CODE_OK)
        {
                av_log(avctx, AV_LOG_ERROR,
                       "imx_vpu_api_dec_push_encoded_frame() failed: %s\n",
                       imx_vpu_api_dec_return_code_string(dec_ret));
                return 1;
        }
        return 0;
}

static int new_stream_available(AVCodecContext *avctx, imxvpuapiDecContext *ctx,
                                ImxVpuApiRawFrame decoded_frame)
{
        ImxVpuApiDecStreamInfo const *stream_info;
        int ret = 0;

        stream_info = imx_vpu_api_dec_get_stream_info(ctx->decoder);

        deallocate_framebuffers(ctx);
        ctx->stream_info = *stream_info;
        ret = allocate_and_add_fb_pool_framebuffers(avctx, ctx,
                                                    stream_info->min_num_required_framebuffers,
                                                    decoded_frame);
        if (ret)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not allocate %zu framebuffer(s)\n",
                       stream_info->min_num_required_framebuffers);
                return 1;
        }

        if (!(ctx->dec_global_info->flags &
              IMX_VPU_API_DEC_GLOBAL_INFO_FLAG_DECODED_FRAMES_ARE_FROM_BUFFER_POOL))
        {
                ret = allocate_output_framebuffer(avctx, ctx);
                if (ret)
                {
                        av_log(avctx, AV_LOG_ERROR, "Could not allocate output framebuffer\n");
                        return 1;
                }
                imx_vpu_api_dec_set_output_frame_dma_buffer(ctx->decoder,
                                                            ctx->output_dmabuffer,
                                                            decoded_frame.fb_context);
        }
        return 0;
}

static int decoded_frame_available(AVCodecContext *avctx, imxvpuapiDecContext
                                   *ctx, ImxVpuApiRawFrame decoded_frame,
                                   AVFrame *frame)
{
        ImxVpuApiFramebufferMetrics const *fb_metrics;
        uint8_t *virtual_address, *y_src, *uv_src;
        ImxVpuApiDecReturnCodes dec_ret;
        int err = 0, y;

        dec_ret = imx_vpu_api_dec_get_decoded_frame(ctx->decoder,
                                                    &decoded_frame);
        if (dec_ret)
        {
                av_log(avctx, AV_LOG_ERROR, "imx_vpu_api_dec_get_decoded_frame() failed: %s\n",
                       imx_vpu_api_dec_return_code_string(dec_ret));
                return 1;
        }

        virtual_address = imx_dma_buffer_map(decoded_frame.fb_dma_buffer,
                                             IMX_DMA_BUFFER_MAPPING_FLAG_READ,
                                             &err);

        fb_metrics = &(ctx->stream_info.decoded_frame_framebuffer_metrics);
        y_src = virtual_address + fb_metrics->y_offset;
        uv_src = virtual_address + fb_metrics->u_offset;

        for(y = 0; y < avctx->height; y++)
        {
                memcpy(frame->data[0] + y * avctx->width, y_src, avctx->width);
                y_src += fb_metrics->y_stride;
        }
        for(y = 0; y < avctx->height; y++)
        {
                memcpy(frame->data[1] + y * avctx->width / 2, uv_src,
                       avctx->width / 2);
                uv_src += fb_metrics->uv_stride / 2;
        }
        imx_dma_buffer_unmap(decoded_frame.fb_dma_buffer);

        imx_vpu_api_dec_return_framebuffer_to_decoder(ctx->decoder,
                                                      decoded_frame.fb_dma_buffer);
        return 0;
}

static int decode_frame(AVCodecContext *avctx, imxvpuapiDecContext *ctx,
                        AVPacket *avpkt, AVFrame *frame)
{
        ImxVpuApiDecSkippedFrameReasons reason;
        ImxVpuApiDecOutputCodes output_code;
        ImxVpuApiRawFrame decoded_frame;
        ImxVpuApiDecReturnCodes dec_ret;
        int ret = 0, do_loop = 1;
        uint64_t pts, dts;
        void *context;

        if (ctx->output_dmabuffer != NULL){
                imx_vpu_api_dec_set_output_frame_dma_buffer(ctx->decoder,
                                                            ctx->output_dmabuffer,
                                                            decoded_frame.fb_context);
        }
        do
        {
                dec_ret = imx_vpu_api_dec_decode(ctx->decoder, &output_code);
                if (dec_ret != IMX_VPU_API_DEC_RETURN_CODE_OK)
                {
                        av_log(avctx, AV_LOG_ERROR, "imx_vpu_dec_decode() failed: %s\n",
                               imx_vpu_api_dec_return_code_string(dec_ret));
                        return 1;
                }
                switch (output_code)
                {
                        case IMX_VPU_API_DEC_OUTPUT_CODE_NO_OUTPUT_YET_AVAILABLE:
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_EOS:
                                do_loop = 0;
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_NEW_STREAM_INFO_AVAILABLE:
                                ret = new_stream_available(avctx, ctx,
                                                           decoded_frame);
                                if (ret)
                                        return 1;
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_NEED_ADDITIONAL_FRAMEBUFFER:
                                ret = allocate_and_add_fb_pool_framebuffers(avctx,
                                                                            ctx,
                                                                            1,
                                                                            decoded_frame);
                                if (ret)
                                        return 1;
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_DECODED_FRAME_AVAILABLE:
                                ret = decoded_frame_available(avctx, ctx,
                                                              decoded_frame,
                                                              frame);
                                if (ret)
                                        return 1;
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_MORE_INPUT_DATA_NEEDED:
                                do_loop = 0;
                                break;

                        case IMX_VPU_API_DEC_OUTPUT_CODE_FRAME_SKIPPED:
                                imx_vpu_api_dec_get_skipped_frame_info(ctx->decoder,
                                                                       &reason,
                                                                       &context,
                                                                       &pts,
                                                                       &dts);
                                av_log(avctx, AV_LOG_ERROR, "frame got skipped: reason %s context %p PTS %" PRIu64 " DTS %" PRIu64 "\n",
                                       imx_vpu_api_dec_skipped_frame_reason_string(reason),
                                       context, pts, dts);
                                break;

                        default:
                                av_log(avctx, AV_LOG_ERROR, "UNKNOWN OUTPUT CODE %s (%d)\n",
                                       imx_vpu_api_dec_output_code_string(output_code),
                                       (int)output_code);
                                return 1;
                }
        }
        while (do_loop);

        return 0;
}

int ff_imxvpuapi_dec_init(AVCodecContext *avctx, imxvpuapiDecContext *ctx)
{
        ImxVpuApiDecOpenParams open_params;
        ImxVpuApiDecReturnCodes dec_ret;
        int err = 0;

        if (avctx->pix_fmt != AV_PIX_FMT_YUV420P)
        {
                av_log(avctx, AV_LOG_ERROR, "Unsupported pixel format\n");
                return -1;
        }

        ctx->dec_global_info = imx_vpu_api_dec_get_global_info();

        ctx->allocator = imx_dma_buffer_allocator_new(&err);
        if (ctx->allocator == NULL)
        {
                av_log(avctx, AV_LOG_ERROR, "could not create DMA buffer allocator: %s (%d)\n",
                       strerror(err), err);
                return -1;
        }

        memset(&open_params, 0, sizeof(open_params));
        open_params.compression_format = ctx->compression_format;
        open_params.flags =
                IMX_VPU_API_DEC_OPEN_PARAMS_FLAG_ENABLE_FRAME_REORDERING
                | IMX_VPU_API_DEC_OPEN_PARAMS_FLAG_USE_SEMI_PLANAR_COLOR_FORMAT;
        open_params.frame_width = avctx->width;
        open_params.frame_height = avctx->height;

        ctx->stream_buffer = imx_dma_buffer_allocate(ctx->allocator,
                                                     ctx->dec_global_info->min_required_stream_buffer_size,
                                                     ctx->dec_global_info->required_stream_buffer_physaddr_alignment,
                                                     0);
        dec_ret = imx_vpu_api_dec_open(&(ctx->decoder), &open_params,
                                       ctx->stream_buffer);
        if (dec_ret != IMX_VPU_API_DEC_RETURN_CODE_OK)
        {
                av_log(avctx, AV_LOG_ERROR, "could not open decoder instance: %s\n",
                       imx_vpu_api_dec_return_code_string(dec_ret));
                return -1;
        }
        return 0;
}

int ff_imxvpuapi_dec_frame(AVCodecContext *avctx, imxvpuapiDecContext *ctx, void
                           *data, int *got_frame, AVPacket *avpkt)
{
        AVFrame *frame = data;
        int ret;

        //There is only one output format available
        avctx->pix_fmt = AV_PIX_FMT_NV12;

        ret = ff_get_buffer(avctx, frame, 0);
        if (ret < 0)
                return -1;

        if (avpkt->data)
        {
                if (push_encoded_input_frame(avctx, ctx, avpkt))
                        return -1;

                if (decode_frame(avctx, ctx, avpkt, frame))
                        return -1;

                *got_frame = 1;
        }
        return 0;
}

int ff_imxvpuapi_dec_close(AVCodecContext *avctx, imxvpuapiDecContext *ctx)
{
        if (ctx == NULL)
                return 0;

        if (ctx->decoder != NULL)
        {
                imx_vpu_api_dec_close(ctx->decoder);
                ctx->decoder = NULL;
        }

        deallocate_framebuffers(ctx);
        if (ctx->output_dmabuffer != NULL)
        {
                imx_dma_buffer_deallocate(ctx->output_dmabuffer);
                ctx->output_dmabuffer = NULL;
        }
        if(ctx->stream_buffer != NULL)
        {
                imx_dma_buffer_deallocate(ctx->stream_buffer);
                ctx->stream_buffer = NULL;
        }

        if (ctx->allocator != NULL)
        {
                imx_dma_buffer_allocator_destroy(ctx->allocator);
                ctx->allocator = NULL;
        }

        return 0;
}
