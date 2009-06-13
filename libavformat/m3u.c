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
    printf("m3u read header called\n");
    PlaylistD *playld = av_malloc(sizeof(PlaylistD));
    ff_split_wd_fn(s->filename,
                &playld->workingdir,
                &playld->filename);
    m3u_list_files(s->pb,
                   &(playld->flist),
                   &(playld->pelist_size),
                   playld->workingdir);
//    playld = av_make_playlistd(flist, flist_len);
    playld->pelist = av_malloc(playld->pelist_size * sizeof(PlayElem*));
    memset(playld->pelist, 0, playld->pelist_size * sizeof(PlayElem*));
    playld->pe_curidx = 0;
    s->priv_data = playld;
    ff_playlist_populate_context(playld, s);
    return 0;
}

static int m3u_read_packet(AVFormatContext *s,
                           AVPacket *pkt)
{
    int i;
    int ret;
    int time_offset = 0;
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    retr:
    ic = playld->pelist[playld->pe_curidx]->ic;
    ret = ic->iformat->read_packet(ic, pkt);
//    if (pkt) {
//        pkt->stream_index += get_stream_offset(s);
//    }
    if (ret < 0 && playld->pe_curidx < playld->pelist_size - 1)
    {
        ++playld->pe_curidx;
//        pkt->destruct(pkt);
        pkt = av_malloc(sizeof(AVPacket));
        ff_playlist_populate_context(playld, s);
        goto retr;
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
    ic = playld->pelist[playld->pe_curidx]->ic;
    ic->iformat->read_seek(ic, stream_index, pts, flags);
}

static int m3u_read_play(AVFormatContext *s)
{
    printf("m3u_read_play called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidx]->ic;
    return av_read_play(ic);
}

static int m3u_read_pause(AVFormatContext *s)
{
    printf("m3u_read_pause called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidx]->ic;
    return av_read_pause(ic);
}

static int m3u_read_close(AVFormatContext *s)
{
    printf("m3u_read_close called\n");
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    ic = playld->pelist[playld->pe_curidx]->ic;
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
    ic = playld->pelist[playld->pe_curidx]->ic;
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
