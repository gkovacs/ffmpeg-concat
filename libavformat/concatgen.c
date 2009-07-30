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
    PlaylistContext *ctx;
    AVFormatContext *ic;
    char have_switched_streams = 0;
    ctx = s->priv_data;
    stream_index = 0;
    for (;;) {
        ic = ctx->icl[ctx->pe_curidx];
        ret = ic->iformat->read_packet(ic, pkt);
        if (pkt) {
            stream_index = pkt->stream_index;
            pkt->stream = ic->streams[pkt->stream_index];
        }
        if (ret >= 0) {
            if (pkt) {
                // TODO changing either dts or pts leads to timing issues on h264
                pkt->dts += av_rescale_q(ff_playlist_time_offset(ctx->durations, ctx->pe_curidx),
                                         AV_TIME_BASE_Q,
                                         ic->streams[stream_index]->time_base);
                if (!ic->streams[pkt->stream_index]->codec->has_b_frames)
                    pkt->pts = pkt->dts + 1;
            }
            break;
        } else {
            if (!have_switched_streams && ctx->pe_curidx < ctx->pelist_size - 1 && ic->cur_st) {
            // TODO switch from AVERROR_EOF to AVERROR_EOS
            // -32 AVERROR_EOF for avi, -51 for ogg
                av_log(ic, AV_LOG_DEBUG, "Switching stream %d to %d\n", stream_index, ctx->pe_curidx+1);
                ctx->durations[ctx->pe_curidx] = ic->duration;
                ctx->pe_curidx = ff_playlist_stream_index_from_time(ctx, ff_playlist_time_offset(ctx->durations, ctx->pe_curidx));
                ff_playlist_populate_context(ctx, ctx->pe_curidx);
                ff_playlist_set_streams(s);
                have_switched_streams = 1;
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
    PlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    int64_t pts_avtimebase = av_rescale_q(pts, ic->cur_st->time_base, AV_TIME_BASE_Q);
    fprintf(stderr, "\n\n\npts_avtimebase is %ld\n\n\n", pts_avtimebase);
    fflush(stderr);
    ctx->pe_curidx = ff_playlist_stream_index_from_time(ctx, pts_avtimebase);
    fprintf(stderr, "\n\n\npe curidx is %d\n\n\n", ctx->pe_curidx);
    fflush(stderr);
    if (!ctx->icl[ctx->pe_curidx]) {
        fprintf(stderr, "\n\n\nswitching files\n\n\n");
        fflush(stderr);
        ff_playlist_populate_context(ctx, ctx->pe_curidx);
        ff_playlist_set_streams(s);
    }
    ic = ctx->icl[ctx->pe_curidx];
    int64_t pts_remain_avtimebase = ff_playlist_remainder_from_time(ctx, pts_avtimebase);
    fprintf(stderr, "\n\n\npts_remain_avtimebase is %ld\n\n\n", pts_remain_avtimebase);
    int64_t pts_remain = av_rescale_q(pts_remain_avtimebase, AV_TIME_BASE_Q, ic->streams[stream_index]->time_base);
    fprintf(stderr, "\n\n\npts_remain is %ld\n\n\n", pts_remain);
    return ic->iformat->read_seek(ic, stream_index, 0, flags);
//    return ic->iformat->read_seek(ic, stream_index, pts_remain, flags);
}

int64_t ff_concatgen_read_timestamp(AVFormatContext *s,
                                    int stream_index,
                                    int64_t *pos,
                                    int64_t pos_limit)
{
    PlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    if (ic->iformat->read_timestamp)
        return ic->iformat->read_timestamp(ic, stream_index, pos, pos_limit);
    return 0;
}

int ff_concatgen_read_close(AVFormatContext *s)
{
    PlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    if (ic->iformat->read_close)
        return ic->iformat->read_close(ic);
    return 0;
}

int ff_concatgen_read_play(AVFormatContext *s)
{
    PlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    return av_read_play(ic);
}

int ff_concatgen_read_pause(AVFormatContext *s)
{
    PlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = s->priv_data;
    ic = ctx->icl[ctx->pe_curidx];
    return av_read_pause(ic);
}
