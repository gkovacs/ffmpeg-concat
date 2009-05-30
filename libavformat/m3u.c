/*
 * M3U muxer and demuxer
 * Copyright (c) 2001 Geza Kovacs
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

/*
 * Based on AU muxer and demuxer in au.c
 */

#include "avformat.h"
#include "raw.h"
#include "riff.h"
#include "playlist.h"

/* if we don't know the size in advance */
#define M3U_UNKNOWN_SIZE ((uint32_t)(~0))

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_m3u_tags[] = {
    { CODEC_ID_PCM_MULAW, 1 },
    { CODEC_ID_PCM_S8, 2 },
    { CODEC_ID_PCM_S16BE, 3 },
    { CODEC_ID_PCM_S24BE, 4 },
    { CODEC_ID_PCM_S32BE, 5 },
    { CODEC_ID_PCM_F32BE, 6 },
    { CODEC_ID_PCM_F64BE, 7 },
    { CODEC_ID_PCM_ALAW, 27 },
    { 0, 0 },
};



#if CONFIG_PLAYLIST_MUXER
/* AUDIO_FILE header */
static int put_m3u_header(ByteIOContext *pb, AVCodecContext *enc)
{
    if(!enc->codec_tag)
        return -1;
    put_tag(pb, ".snd");       /* magic number */
    put_be32(pb, 24);           /* header size */
    put_be32(pb, M3U_UNKNOWN_SIZE); /* data size */
    put_be32(pb, (uint32_t)enc->codec_tag);     /* codec ID */
    put_be32(pb, enc->sample_rate);
    put_be32(pb, (uint32_t)enc->channels);
    return 0;
}

static int m3u_write_header(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;

    s->priv_data = NULL;

    /* format header */
    if (put_m3u_header(pb, s->streams[0]->codec) < 0) {
        return -1;
    }

    put_flush_packet(pb);

    return 0;
}

static int m3u_write_packet(AVFormatContext *s, AVPacket *pkt)
{
    ByteIOContext *pb = s->pb;
    put_buffer(pb, pkt->data, pkt->size);
    return 0;
}

static int m3u_write_trailer(AVFormatContext *s)
{
    ByteIOContext *pb = s->pb;
    int64_t file_size;

    if (!url_is_streamed(s->pb)) {

        /* update file size */
        file_size = url_ftell(pb);
        url_fseek(pb, 8, SEEK_SET);
        put_be32(pb, (uint32_t)(file_size - 24));
        url_fseek(pb, file_size, SEEK_SET);

        put_flush_packet(pb);
    }

    return 0;
}
#endif /* CONFIG_M3U_MUXER */



static int compare_bufs(unsigned char *buf, unsigned char *rbuf)
{
    while (*rbuf != 0)
    {
        if (*rbuf != *buf)
            return 0;
        ++buf;
        ++rbuf;
    }
    return 1;
}

static int m3u_probe(AVProbeData *p)
{
    if (p->buf_size >= 7 && p->buf != 0)
    {
        if (compare_bufs(p->buf, "#EXTM3U"))
            return AVPROBE_SCORE_MAX;
        else
            return 0;
    }
    if (check_file_extn(p->filename, "m3u"))
        return AVPROBE_SCORE_MAX/2;
    else
        return 0;
}

static int m3u_list_files(unsigned char *buffer, int buffer_size, unsigned char **file_list, unsigned int *lfx_ptr)
{
    unsigned int i;
    unsigned int lfx = 0;
    unsigned int fx = 0;
    unsigned char hashed_out = 0;
    unsigned char fldata[262144];
    memset(fldata, 0, 262144);
    memset(file_list, 0, 512 * sizeof(unsigned char*));
    for (i = 0; i < 512; ++i)
    {
        file_list[i] = fldata+i*512;
    }
    for (i = 0; i < buffer_size; ++i)
    {
        if (buffer[i] == 0)
            break;
        if (buffer[i] == '#')
        {
            hashed_out = 1;
            continue;
        }
        if (buffer[i] == '\n')
        {
            hashed_out = 0;
            if (fx != 0)
            {
                fx = 0;
                ++lfx;
            }
            continue;
        }
        if (hashed_out)
            continue;
        file_list[lfx][fx] = buffer[i];
        ++fx;
    }
    *lfx_ptr = lfx;
    return 0;
}



/* m3u input */
static int m3u_read_header(AVFormatContext *s,
                          AVFormatParameters *ap)
{
    unsigned char *flist[512];
    int flist_len;
    ByteIOContext *pb;
    PlaylistD *playld;
    pb = s->pb;
    m3u_list_files(pb->buffer, pb->buffer_size, flist, &flist_len);
    playld = av_make_playlistd(flist, flist_len);
    s->priv_data = playld;
    playlist_populate_context(playld, s);
    return 0;
}

#define MAX_SIZE 4096

static int m3u_read_packet(AVFormatContext *s,
                          AVPacket *pkt)
{
    int ret;
    PlaylistD *playld;
    AVFormatContext *ic;
    playld = s->priv_data;
    retr:
    ic = playld->pelist[playld->pe_curidx]->ic;
    ret = ic->iformat->read_packet(ic, pkt);
    if (ret < 0 && playld->pe_curidx < playld->pelist_size - 1)
    {
        ++playld->pe_curidx;
        // TODO clear all existing streams before repopulating
        playlist_populate_context(playld, s);
        goto retr;
    }
    return ret;
}

#if CONFIG_M3U_DEMUXER
AVInputFormat m3u_demuxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("M3U format"),
    0,
    m3u_probe,
    m3u_read_header,
    m3u_read_packet,
    NULL,
    pcm_read_seek,
    .codec_tag= (const AVCodecTag* const []){codec_m3u_tags, 0},
};
#endif

#if CONFIG_M3U_MUXER
AVOutputFormat m3u_muxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("M3U format"),
    "audio/x-mpegurl",
    "m3u",
    0,
    CODEC_ID_PCM_S16BE,
    CODEC_ID_NONE,
    m3u_write_header,
    m3u_write_packet,
    m3u_write_trailer,
    .codec_tag= (const AVCodecTag* const []){codec_m3u_tags, 0},
};
#endif //CONFIG_M3U_MUXER
