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

#include "concatgen.h"

int concatgen_read_packet(AVFormatContext *s,
                       AVPacket *pkt)
{
    int i;
    int ret;
    int stream_index;
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    stream_index = 0;
    retr:
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    ret = ic->iformat->read_packet(ic, pkt);
    if (pkt) {
        stream_index = pkt->stream_index;
        ic = playld->pelist[playld->pe_curidxs[stream_index]]->ic;
    }
    if (ret >= 0) {
        if (pkt) {
            int64_t time_offset = ff_conv_stream_time(ic, pkt->stream_index, playld->time_offsets[pkt->stream_index]);
            pkt->dts += time_offset;
            if (!ic->streams[pkt->stream_index]->codec->has_b_frames)
                pkt->pts = pkt->dts + 1;
        }
    }
    // TODO switch from AVERROR_EOF to AVERROR_EOS
    // -32 AVERROR_EOF for avi, -51 for ogg
    else if (ret < 0 && playld->pe_curidxs[stream_index] < playld->pelist_size - 1) {
        // TODO account for out-of-sync audio/video by using per-stream offsets
        // using streams[]->duration slightly overestimates offset
//        playld->dts_offset += ic->streams[0]->duration;
        // using streams[]->cur_dts slightly overestimates offset
//        playld->dts_offset += ic->streams[0]->cur_dts;
//        playld->dts_offset += playld->dts_prevpacket;
        printf("switching streams\n");
        for (i = 0; i < ic->nb_streams && i < playld->time_offsets_size; ++i) {
            playld->time_offsets[i] += ff_get_duration(ic, i);
        }
        ++playld->pe_curidxs[stream_index];
//        pkt->destruct(pkt);
        pkt = av_malloc(sizeof(AVPacket));
//        for (i = 0; i < playld->pe_curidxs_size; ++i) {
            ff_playlist_populate_context(playld, s, stream_index);
//        }
        goto retr;
    }
    else {
        printf("avpacket ret is %d\n", ret);
    }
    return ret;
}

int concatgen_read_seek(AVFormatContext *s,
                     int stream_index,
                     int64_t pts,
                     int flags)
{
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    ic->iformat->read_seek(ic, stream_index, pts, flags);
}

int concatgen_read_timestamp(AVFormatContext *s,
                             int stream_index,
                             int64_t *pos,
                             int64_t pos_limit)
{
    printf("m3u_read_timestamp called\n");
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    if (ic->iformat->read_timestamp)
        return ic->iformat->read_timestamp(ic, stream_index, pos, pos_limit);
    return 0;
}

int concatgen_read_close(AVFormatContext *s)
{
    printf("m3u_read_close called\n");
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    if (ic->iformat->read_close)
        return ic->iformat->read_close(ic);
    return 0;
}

int concatgen_read_play(AVFormatContext *s)
{
    printf("m3u_read_play called\n");
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    return av_read_play(ic);
}

int concatgen_read_pause(AVFormatContext *s)
{
    printf("m3u_read_pause called\n");
    PlaylistC *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    return av_read_pause(ic);
}
