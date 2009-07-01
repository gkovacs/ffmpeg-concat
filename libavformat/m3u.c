/*
 * M3U muxer and demuxer
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

#include "avformat.h"
#include "riff.h"
#include "playlist.h"

/* if we don't know the size in advance */
#define M3U_UNKNOWN_SIZE ((uint32_t)(~0))

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_m3u_tags[] = {
    { 0, 0 },
};

static int m3u_probe(AVProbeData *p)
{
    if (p->buf != 0) {
        if (!strncmp(p->buf, "#EXTM3U", 7))
            return AVPROBE_SCORE_MAX;
        else
            return 0;
    }
    if (match_ext(p->filename, "m3u"))
        return AVPROBE_SCORE_MAX/2;
    else
        return 0;
}

static int m3u_list_files(ByteIOContext *s,
                          char ***flist_ptr,
                          unsigned int *lfx_ptr,
                          char *workingdir)
{
    char **ofl;
    int i;
    int bufsize = 16;
    i = 0;
    ofl = av_malloc(sizeof(char*) * bufsize);
    while (1) {
        char *c = ff_buf_getline(s);
        if (c == NULL) // EOF
            break;
        if (*c == 0) // hashed out
            continue;
        ofl[i] = c;
        if (++i == bufsize) {
            bufsize += 16;
            ofl = av_realloc(ofl, sizeof(char*) * bufsize);
        }
    }
    *flist_ptr = ofl;
    *lfx_ptr = i;
    ofl[i] = 0;
    while (*ofl != 0) { // determine if relative paths
        FILE *file;
        char *fullfpath = ff_conc_strings(workingdir, *ofl);
        file = fopen(fullfpath, "r");
        if (file) {
            fclose(file);
            *ofl = fullfpath;
        }
        ++ofl;
    }
    return 0;
}

static int m3u_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    int i;
    PlaylistD *playld = ff_make_playlistd(s->filename);
    m3u_list_files(s->pb,
                   &(playld->flist),
                   &(playld->pelist_size),
                   playld->workingdir);
    playld->pelist = av_malloc(playld->pelist_size * sizeof(PlayElem*));
    memset(playld->pelist, 0, playld->pelist_size * sizeof(PlayElem*));
    s->priv_data = playld;
    for (i = 0; i < playld->pe_curidxs_size; ++i) {
        ff_playlist_populate_context(playld, s, i);
    }
    return 0;
}

static int m3u_read_packet(AVFormatContext *s,
                           AVPacket *pkt)
{
    int i;
    int ret;
    int stream_index;
    PlaylistD *playld;
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
            pkt->dts += ff_conv_stream_time(ic, pkt->stream_index, playld->time_offsets[pkt->stream_index]);
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

static int m3u_read_seek(AVFormatContext *s,
                         int stream_index,
                         int64_t pts,
                         int flags)
{
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    ic->iformat->read_seek(ic, stream_index, pts, flags);
}

static int m3u_read_play(AVFormatContext *s)
{
    printf("m3u_read_play called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    return av_read_play(ic);
}

static int m3u_read_pause(AVFormatContext *s)
{
    printf("m3u_read_pause called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    return av_read_pause(ic);
}

static int m3u_read_close(AVFormatContext *s)
{
    printf("m3u_read_close called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    if (ic->iformat->read_close)
        return ic->iformat->read_close(ic);
    return 0;
}

static int m3u_read_timestamp(AVFormatContext *s,
                              int stream_index,
                              int64_t *pos,
                              int64_t pos_limit)
{
    printf("m3u_read_timestamp called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidxs[0]]->ic;
    if (ic->iformat->read_timestamp)
        return ic->iformat->read_timestamp(ic, stream_index, pos, pos_limit);
    return 0;
}

#if CONFIG_M3U_DEMUXER
AVInputFormat m3u_demuxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("M3U format"),
    0,
    m3u_probe,
    m3u_read_header,
    m3u_read_packet,
    m3u_read_close, //m3u_read_close
    m3u_read_seek,
    m3u_read_timestamp, //m3u_read_timestamp
    NULL, //flags
    NULL, //extensions
    NULL, //value
    m3u_read_play,
    m3u_read_pause,
    (const AVCodecTag* const []){codec_m3u_tags, 0},
    NULL, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
