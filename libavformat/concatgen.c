/*
 * Generic functions used by playlist/concatenation demuxers
 * Copyright (c) 2009 Geza Kovacs
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

/** @file libavformat/concatgen.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief Generic functions used by playlist/concatenation demuxers
 *
 *  @details These functions are used to read packets and seek streams
 *  for concat-type demuxers, abstracting away the playlist element switching
 *  process.
 */

#include "concatgen.h"
#include "avformat.h"
#include "avplaylist.h"

int ff_concatgen_read_packet(AVFormatContext *s,
                             AVPacket *pkt)
{
    int ret, i, stream_index;
    char have_switched_streams = 0;
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic;
    stream_index = 0;
    for (;;) {
        ic = ctx->formatcontext_list[ctx->pe_curidx];
        av_init_packet(pkt);
        if (s->packet_buffer) {
            *pkt = s->packet_buffer->pkt;
            s->packet_buffer = s->packet_buffer->next;
            ret = 0;
        } else {
            ret = ic->iformat->read_packet(ic, pkt);
        }
        if (ret >= 0) {
            if (pkt) {
                int streams_offset = av_playlist_streams_offset_from_playidx(ctx, ctx->pe_curidx);
                stream_index = av_playlist_localstidx_from_streamidx(ctx, pkt->stream_index);
                pkt->stream_index = stream_index + streams_offset;
                if (!ic->streams[stream_index]->codec->has_b_frames ||
                    ic->streams[stream_index]->codec->codec->id == CODEC_ID_MPEG1VIDEO) {
                    int64_t time_offset_localbase = av_rescale_q(av_playlist_time_offset(ctx->durations, streams_offset),
                                                                 AV_TIME_BASE_Q,
                                                                 ic->streams[stream_index]->time_base);
                    pkt->dts += time_offset_localbase;
                    if (pkt->pts != AV_NOPTS_VALUE)
                        pkt->pts += time_offset_localbase;
                }
            }
            break;
        } else {
            if (!have_switched_streams &&
                ctx->pe_curidx < ctx->pelist_size - 1 &&
                ret != AVERROR(EAGAIN)) {
            // TODO switch from AVERROR_EOF to AVERROR_EOS
            // -32 AVERROR_EOF for avi, -51 for ogg

                av_log(ic, AV_LOG_DEBUG, "Switching stream %d to %d\n", stream_index, ctx->pe_curidx+1);
                ctx->durations[ctx->pe_curidx] = ic->duration;
                ctx->pe_curidx = av_playlist_stream_index_from_time(ctx,
                                                                    av_playlist_time_offset(ctx->durations, ctx->pe_curidx),
                                                                    NULL);
                if (av_playlist_populate_context(ctx, ctx->pe_curidx) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Failed to switch to AVFormatContext %d\n", ctx->pe_curidx);
                    break;
                }
                if ((av_playlist_set_streams(s)) < 0) {
                    av_log(NULL, AV_LOG_ERROR, "Failed to open codecs for streams in %d\n", ctx->pe_curidx);
                    break;
                }
                // have_switched_streams is set to avoid infinite loop
                have_switched_streams = 1;
                // duration is updated in case it's checked by a parent demuxer (chained concat demuxers)
                s->duration = 0;
                for (i = 0; i < ctx->pe_curidx; ++i)
                    s->duration += ctx->durations[i];
                continue;
            } else {
                av_log(ic, AV_LOG_ERROR, "Packet read error %d\n", ret);
                break;
            }
        }
    }
    return ret;
}

int ff_concatgen_read_seek(AVFormatContext *s,
                           int stream_index,
                           int64_t pts,
                           int flags)
{
    int i, err;
    int64_t localpts_avtimebase, localpts, pts_avtimebase;
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic = ctx->formatcontext_list[ctx->pe_curidx];
    ctx->durations[ctx->pe_curidx] = ic->duration;
    pts_avtimebase = av_rescale_q(pts,
                                  ic->streams[stream_index]->time_base,
                                  AV_TIME_BASE_Q);
    ctx->pe_curidx = av_playlist_stream_index_from_time(ctx,
                                                        pts_avtimebase,
                                                        &localpts_avtimebase);
    err = av_playlist_populate_context(ctx, ctx->pe_curidx);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to switch to AVFormatContext %d\n", ctx->pe_curidx);
        return err;
    }
    err = av_playlist_set_streams(s);
    if (err < 0) {
        av_log(NULL, AV_LOG_ERROR, "Failed to open codecs for streams in %d\n", ctx->pe_curidx);
        return err;
    }
    ic = ctx->formatcontext_list[ctx->pe_curidx];
    localpts = av_rescale_q(localpts_avtimebase,
                            AV_TIME_BASE_Q,
                            ic->streams[stream_index]->time_base);
    s->duration = 0;
    for (i = 0; i < ctx->pe_curidx; ++i)
        s->duration += ctx->durations[i];
    return ic->iformat->read_seek(ic, stream_index, localpts, flags);
}

int64_t ff_concatgen_read_timestamp(AVFormatContext *s,
                                    int stream_index,
                                    int64_t *pos,
                                    int64_t pos_limit)
{
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic = ctx->formatcontext_list[ctx->pe_curidx];
    if (ic->iformat->read_timestamp)
        return ic->iformat->read_timestamp(ic, stream_index, pos, pos_limit);
    return 0;
}

int ff_concatgen_read_close(AVFormatContext *s)
{
    int i;
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic;
    for (i = 0; i < ctx->pelist_size; ++i) {
        ic = ctx->formatcontext_list[i];
        if (ic && ic->iformat->read_close)
            return ic->iformat->read_close(ic);
    }
    return 0;
}

int ff_concatgen_read_play(AVFormatContext *s)
{
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic = ctx->formatcontext_list[ctx->pe_curidx];
    return av_read_play(ic);
}

int ff_concatgen_read_pause(AVFormatContext *s)
{
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic = ctx->formatcontext_list[ctx->pe_curidx];
    return av_read_pause(ic);
}
