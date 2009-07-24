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

#include "playlist.h"

int ff_concatgen_read_packet(AVFormatContext *s,
                             AVPacket *pkt)
{
    int ret;
    int stream_index;
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
                int64_t time_offset;
                time_offset = av_rescale_q(ctx->time_offset, AV_TIME_BASE_Q, ic->streams[stream_index]->time_base);
                av_log(ic, AV_LOG_DEBUG, "%s conv stream time from %ld to %d/%d is %ld\n", ic->iformat->name, ctx->time_offset, ic->streams[stream_index]->time_base.num, ic->streams[stream_index]->time_base.den, time_offset);
                // TODO changing either dts or pts leads to timing issues on h264
                pkt->dts += time_offset;
                if (!ic->streams[pkt->stream_index]->codec->has_b_frames)
                    pkt->pts = pkt->dts + 1;
            }
            break;
        } else {
            if (!have_switched_streams && ctx->pe_curidx < ctx->pelist_size - 1 && ic->cur_st) {
            // TODO switch from AVERROR_EOF to AVERROR_EOS
            // -32 AVERROR_EOF for avi, -51 for ogg
                av_log(ic, AV_LOG_DEBUG, "Switching stream %d to %d\n", stream_index, ctx->pe_curidx+1);
                ctx->time_offset += av_rescale_q(ic->cur_st->duration, ic->cur_st->time_base, AV_TIME_BASE_Q);
                if (!ctx->icl[++ctx->pe_curidx])
                    ff_playlist_populate_context(s);
                have_switched_streams = 1;
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
    return ic->iformat->read_seek(ic, stream_index, pts, flags);
}

int ff_concatgen_read_timestamp(AVFormatContext *s,
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
