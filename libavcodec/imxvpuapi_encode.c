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

static ImxVpuApiColorFormat convert_pix_fmt(AVCodecContext *avctx,
                                            imxvpuapiEncContext *ctx)
{
        switch(avctx->pix_fmt){
        case AV_PIX_FMT_YUV420P:
                ctx->uv_height = avctx->height / 2;
                ctx->uv_width = avctx->width / 2;
                return IMX_VPU_API_COLOR_FORMAT_FULLY_PLANAR_YUV420_8BIT;
        case AV_PIX_FMT_YUV422P:
                ctx->uv_height = avctx->height / 2;
                ctx->uv_width = avctx->width;
                return IMX_VPU_API_COLOR_FORMAT_FULLY_PLANAR_YUV422_HORIZONTAL_8BIT;
        };
        return -1;
}

static int allocate_and_add_fb_pool_framebuffers(AVCodecContext *avctx,
                                                  size_t num_fb_pool_framebuffers_to_add,
                                                  imxvpuapiEncContext *ctx)
{
        ImxDmaBuffer **new_fb_pool_dmabuffers_array;
        size_t old_num_fb_pool_framebuffers;
        size_t new_num_fb_pool_framebuffers;
        ImxVpuApiEncReturnCodes enc_ret;
        int err = 0;
        size_t i;

        if (num_fb_pool_framebuffers_to_add == 0)
                return 0;

        old_num_fb_pool_framebuffers = ctx->num_fb_pool_framebuffers;
        new_num_fb_pool_framebuffers = old_num_fb_pool_framebuffers + num_fb_pool_framebuffers_to_add;

        new_fb_pool_dmabuffers_array = realloc(ctx->fb_pool_dmabuffers,
                                               new_num_fb_pool_framebuffers * sizeof(ImxDmaBuffer *));
        assert(new_fb_pool_dmabuffers_array != NULL);
        memset(new_fb_pool_dmabuffers_array + old_num_fb_pool_framebuffers, 0,
               num_fb_pool_framebuffers_to_add * sizeof(ImxDmaBuffer *));

        ctx->fb_pool_dmabuffers = new_fb_pool_dmabuffers_array;
        ctx->num_fb_pool_framebuffers = new_num_fb_pool_framebuffers;

        for (i = old_num_fb_pool_framebuffers; i < ctx->num_fb_pool_framebuffers; ++i)
        {
                ctx->fb_pool_dmabuffers[i] = imx_dma_buffer_allocate(ctx->allocator,
                                                                     ctx->stream_info.min_framebuffer_size,
                                                                     ctx->stream_info.framebuffer_alignment,
                                                                     &err);
                if (ctx->fb_pool_dmabuffers[i] == NULL)
                {
                        av_log(avctx, AV_LOG_ERROR, "Could not allocate DMA buffer for FB pool framebuffer: %s (%d)\n",
                               strerror(err), err);
                        return err;
                }
        }

        enc_ret = imx_vpu_api_enc_add_framebuffers_to_pool(ctx->encoder,
                                                           ctx->fb_pool_dmabuffers + old_num_fb_pool_framebuffers,
                                                           num_fb_pool_framebuffers_to_add);
        if (enc_ret != IMX_VPU_API_ENC_RETURN_CODE_OK)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not add framebuffers to VPU pool: %s\n",
                       imx_vpu_api_enc_return_code_string(enc_ret));
                return enc_ret;
        }
        return 0;
}

static int resize_encoded_frame_buffer(imxvpuapiEncContext *ctx, size_t
                                       new_size)
{
        void *new_buffer;
        if (ctx->encoded_frame_buffer_size >= new_size)
                return 0;
        new_buffer = realloc(ctx->encoded_frame_buffer, new_size);

        ctx->encoded_frame_buffer = new_buffer;
        ctx->encoded_frame_buffer_size = new_size;
        return 0;
}

static int push_raw_input_frame(AVCodecContext *avctx, imxvpuapiEncContext *ctx,
                                const AVFrame *frame)
{
        ImxVpuApiRawFrame raw_input_frame;
        ImxVpuApiFramebufferMetrics const *fb_metrics;
        ImxVpuApiEncReturnCodes enc_ret;
        uint8_t *mapped_virtual_address;
        uint8_t *y_dest, *u_dest, *v_dest;
        size_t y;

        fb_metrics = &(ctx->stream_info.frame_encoding_framebuffer_metrics);
        memset(&raw_input_frame, 0, sizeof(raw_input_frame));
        raw_input_frame.fb_dma_buffer = ctx->input_dmabuffer;

        mapped_virtual_address = imx_dma_buffer_map(ctx->input_dmabuffer,
                                                    IMX_DMA_BUFFER_MAPPING_FLAG_WRITE,
                                                    NULL);
        y_dest = mapped_virtual_address + fb_metrics->y_offset;
        u_dest = mapped_virtual_address + fb_metrics->u_offset;
        v_dest = mapped_virtual_address + fb_metrics->v_offset;

        for(y=0;y<frame->height;y++)
        {
                memcpy(y_dest, frame->data[0] + y * frame->linesize[0],
                       frame->linesize[0]);
                y_dest += fb_metrics->y_stride;
        }

        for(y=0;y<ctx->uv_height;y++)
        {
                memcpy(u_dest, frame->data[1] + y * ctx->uv_width,
                       ctx->uv_width);
                u_dest += fb_metrics->uv_stride;
                memcpy(v_dest, frame->data[2] + y * ctx->uv_width,
                       ctx->uv_width);
                v_dest += fb_metrics->uv_stride;
        }

        imx_dma_buffer_unmap(ctx->input_dmabuffer);

        if ((enc_ret = imx_vpu_api_enc_push_raw_frame(ctx->encoder,
                                                      &raw_input_frame)) !=
            IMX_VPU_API_ENC_RETURN_CODE_OK)
        {
                av_log(avctx, AV_LOG_ERROR, "could not push raw frame into encoder: %s\n",
                       imx_vpu_api_enc_return_code_string(enc_ret));
                return 1;
        }
        return 0;
}

static int encode_frame(AVCodecContext *avctx, imxvpuapiEncContext *ctx,
                        AVPacket *pkt, const AVFrame *frame)
{
        ImxVpuApiEncOutputCodes output_code;
        ImxVpuApiEncodedFrame output_frame;
        ImxVpuApiEncReturnCodes enc_ret;
        size_t encoded_frame_size;
        int ret, do_loop = 1;

        do
        {
                if ((enc_ret = imx_vpu_api_enc_encode(ctx->encoder,
                                                      &encoded_frame_size,
                                                      &output_code)) !=
                    IMX_VPU_API_ENC_RETURN_CODE_OK)
                {
                        av_log(avctx, AV_LOG_ERROR, "imx_vpu_api_enc_encode failed(): %s\n",
                               imx_vpu_api_enc_return_code_string(enc_ret));
                        return 1;
                }
                switch (output_code)
                {
                        case IMX_VPU_API_ENC_OUTPUT_CODE_NO_OUTPUT_YET_AVAILABLE:
                                break;

                        case IMX_VPU_API_ENC_OUTPUT_CODE_NEED_ADDITIONAL_FRAMEBUFFER:
                                if (!allocate_and_add_fb_pool_framebuffers(avctx,
                                                                           1,
                                                                           ctx))
                                {
                                        return 1;
                                }
                                break;

                        case IMX_VPU_API_ENC_OUTPUT_CODE_ENCODED_FRAME_AVAILABLE:
                                resize_encoded_frame_buffer(ctx,
                                                            encoded_frame_size);
                                memset(&output_frame, 0, sizeof(output_frame));
                                output_frame.data = ctx->encoded_frame_buffer;
                                output_frame.data_size = encoded_frame_size;
                                if ((enc_ret =
                                     imx_vpu_api_enc_get_encoded_frame(ctx->encoder,
                                                                       &output_frame))
                                    != IMX_VPU_API_ENC_RETURN_CODE_OK)
                                {
                                        av_log(avctx, AV_LOG_ERROR, "Could not retrieve encoded frame: %s\n",
                                               imx_vpu_api_enc_return_code_string(enc_ret));
                                        return 1;
                                }
                                if ((ret = ff_alloc_packet2(avctx, pkt,
                                                            output_frame.data_size,
                                                            0)) <0)
                                        return ret;

                                memcpy(pkt->data, output_frame.data,
                                       output_frame.data_size);
                                pkt->pts = pkt->dts = frame->pts;
                                break;

                        case IMX_VPU_API_ENC_OUTPUT_CODE_MORE_INPUT_DATA_NEEDED:
                                do_loop = 0;
                                break;

                        case IMX_VPU_API_ENC_OUTPUT_CODE_EOS:
                                do_loop = 0;
                                break;

                        default:
                                av_log(avctx, AV_LOG_ERROR, "UNKNOWN OUTPUT CODE %s (%d)\n",
                                       imx_vpu_api_enc_output_code_string(output_code),
                                       (int)output_code);
                                return 1;
                }
        }
         while (do_loop);
        return 0;
}

static void deallocate_framebuffers(imxvpuapiEncContext *ctx)
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

int ff_imxvpuapi_enc_init(AVCodecContext *avctx, imxvpuapiEncContext *ctx)
{
        ImxVpuApiEncStreamInfo const *enc_stream_info;
        ImxVpuApiEncReturnCodes enc_ret;
        int err = 0;

        ctx->color_format = convert_pix_fmt(avctx, ctx);
        if(ctx->color_format == -1){
                av_log(avctx, AV_LOG_ERROR, "pixel format not supported\n");
                return -1;
        }

        ctx->enc_global_info = imx_vpu_api_enc_get_global_info();

        ctx->allocator = imx_dma_buffer_allocator_new(&err);
        if (ctx->allocator == NULL)
        {
                av_log(avctx, AV_LOG_ERROR,
                       "could not create DMA buffer allocator: %s (%d)\n",
                       strerror(err), err);
                return -1;
        }

        memset(&(ctx->open_params), 0, sizeof(ctx->open_params));
        imx_vpu_api_enc_set_default_open_params(ctx->compression_format,
                                                ctx->color_format,
                                                avctx->width,
                                                avctx->height,
                                                &(ctx->open_params));

        ctx->open_params.frame_rate_numerator = avctx->framerate.num;
        ctx->open_params.frame_rate_denominator = avctx->framerate.den;
        /* Resize bitrate to kbps instead of bps */
        ctx->open_params.bitrate = (avctx->bit_rate) / 1000;
        ctx->open_params.gop_size = avctx->gop_size;
        if(avctx->global_quality / FF_QP2LAMBDA != 0){
                ctx->open_params.bitrate = 0;
                ctx->open_params.quantization = avctx->global_quality / FF_QP2LAMBDA;
        }

        ctx->stream_buffer = imx_dma_buffer_allocate(ctx->allocator,
                                                     ctx->enc_global_info->min_required_stream_buffer_size,
                                                     ctx->enc_global_info->required_stream_buffer_physaddr_alignment,
                                                     0
                                                     );
        enc_ret = imx_vpu_api_enc_open(&(ctx->encoder), &(ctx->open_params),
                                       ctx->stream_buffer);
        if (enc_ret != IMX_VPU_API_ENC_RETURN_CODE_OK)
                return -1;

        enc_stream_info = imx_vpu_api_enc_get_stream_info(ctx->encoder);
        ctx->stream_info = *enc_stream_info;

        err = allocate_and_add_fb_pool_framebuffers(avctx,
                                                    enc_stream_info->min_num_required_framebuffers, ctx);
        if (err)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not allocate %zu framebuffer(s)\n",
                       enc_stream_info->min_num_required_framebuffers);
                return -1;
        }

        ctx->input_dmabuffer = imx_dma_buffer_allocate(ctx->allocator,
                                                       enc_stream_info->min_framebuffer_size,
                                                       enc_stream_info->framebuffer_alignment,
                                                       &err);
        if (ctx->input_dmabuffer == NULL)
        {
                av_log(avctx, AV_LOG_ERROR, "Could not allocate DMA buffer for input framebuffer: %s (%d)\n",
                       strerror(err), err);
                return -1;
        }
        return 0;
}

int ff_imxvpuapi_enc_frame(AVCodecContext *avctx, imxvpuapiEncContext *ctx,
                           AVPacket *pkt, const AVFrame *frame, int *got_packet)
{
        if (frame)
        {
                if (push_raw_input_frame(avctx, ctx, frame))
                        return -1;

                if (encode_frame(avctx, ctx, pkt, frame))
                        return -1;

                *got_packet = 1;
        }

        return 0;
}

int ff_imxvpuapi_enc_close(AVCodecContext *avctx, imxvpuapiEncContext *ctx)
{
        if (ctx == NULL)
                return 0;

        if (ctx->encoder != NULL)
                imx_vpu_api_enc_close(ctx->encoder);

        deallocate_framebuffers(ctx);
        if (ctx->input_dmabuffer)
                imx_dma_buffer_deallocate(ctx->input_dmabuffer);
        if (ctx->stream_buffer != NULL)
                imx_dma_buffer_deallocate(ctx->stream_buffer);
        if (ctx->allocator != NULL)
                imx_dma_buffer_allocator_destroy(ctx->allocator);

        return 0;
}
