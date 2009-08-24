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

int ff_concatgen_read_packet(AVFormatContext *s,
                             AVPacket *pkt)
{
    int ret, i, stream_index;
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    char have_switched_streams = 0;
    ctx = s->priv_data;
    stream_index = 0;
    for (;;) {
        ic = ctx->icl[ctx->pe_curidx];
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
                    static int cached_streams_offset = -1;
                    static int64_t time_offset_avbase = 0;
                    int time_offset_localbase;
                    if (cached_streams_offset != streams_offset) { // must recompute timestamp offset
                        cached_streams_offset = streams_offset;
                        time_offset_avbase = av_playlist_time_offset(ctx->durations, streams_offset);
                    }
                    time_offset_localbase = av_rescale_q(time_offset_avbase,
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
                ctx->pe_curidx < ctx->pelist_size - 1) {
            // TODO switch from AVERROR_EOF to AVERROR_EOS
            // -32 AVERROR_EOF for avi, -51 for ogg

                av_log(ic, AV_LOG_DEBUG, "Switching stream %d to %d\n", stream_index, ctx->pe_curidx+1);
                ctx->durations[ctx->pe_curidx] = ic->duration;
                ctx->pe_curidx = av_playlist_stream_index_from_time(ctx,
                                                                    av_playlist_time_offset(ctx->durations, ctx->pe_curidx),
                                                                    NULL);
                av_playlist_populate_context(ctx, ctx->pe_curidx);
                av_playlist_set_streams(s);
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
    int i;
    int64_t localpts_avtimebase, localpts, pts_avtimebase;
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    ctx->durations[ctx->pe_curidx] = ic->duration;
    pts_avtimebase = av_rescale_q(pts,
                                  ic->streams[stream_index]->time_base,
                                  AV_TIME_BASE_Q);
    ctx->pe_curidx = av_playlist_stream_index_from_time(ctx,
                                                        pts_avtimebase,
                                                        &localpts_avtimebase);
    av_playlist_populate_context(ctx, ctx->pe_curidx);
    av_playlist_set_streams(s);
    ic = ctx->icl[ctx->pe_curidx];
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
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    if (ic->iformat->read_timestamp)
        return ic->iformat->read_timestamp(ic, stream_index, pos, pos_limit);
    return 0;
}

int ff_concatgen_read_close(AVFormatContext *s)
{
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    if (ic->iformat->read_close)
        return ic->iformat->read_close(ic);
    return 0;
}

int ff_concatgen_read_play(AVFormatContext *s)
{
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    return av_read_play(ic);
}

int ff_concatgen_read_pause(AVFormatContext *s)
{
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    return av_read_pause(ic);
}
