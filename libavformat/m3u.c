/*
 * M3U playlist demuxer
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

/** @file libavformat/m3u.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief M3U playlist demuxer
 */

#include "playlist.h"

static int ff_concatgen_read_packet(AVFormatContext *s, AVPacket *pkt);

static int ff_concatgen_read_seek(AVFormatContext *s, int stream_index, int64_t pts, int flags);

static int ff_concatgen_read_timestamp(AVFormatContext *s, int stream_index, int64_t *pos, int64_t pos_limit);

static int ff_concatgen_read_close(AVFormatContext *s);

static int ff_concatgen_read_play(AVFormatContext *s);

static int ff_concatgen_read_pause(AVFormatContext *s);

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

static int m3u_list_files(ByteIOContext *s, PlaylistContext *ctx, const char *filename)
{
    char **flist;
    int i, bufsize;
    i = bufsize = flist = 0;
    while (1) {
        char *q;
        char linebuf[1024] = {0};
        if ((q = url_fgets(s, linebuf, sizeof(linebuf))) == NULL) // EOF
            break;
        linebuf[sizeof(linebuf)-1] = 0; // cut off end if buffer overflowed
        while (*q != 0) {
            if (*q++ == '#')
                *(q-1) = 0;
        }
        if (*linebuf == 0) // hashed out
            continue;
        flist = av_fast_realloc(flist, &bufsize, i+2);
        flist[i] = av_malloc(q-linebuf+1);
        av_strlcpy(flist[i], linebuf, q-linebuf+1);
        flist[i++][q-linebuf] = 0;
    }
    flist[i] = 0;
    ff_playlist_relative_paths(flist, i, dirname(filename));
    ff_playlist_add_stringlist(ctx, flist, i);
    av_free(flist);
    return 0;
}

static int m3u_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    int i;
    PlaylistContext *ctx = ff_playlist_alloc_context();
    m3u_list_files(s->pb, ctx, s->filename);
    s->priv_data = ctx;
    ff_playlist_populate_context(s);
    return 0;
}

#if CONFIG_M3U_DEMUXER
AVInputFormat m3u_demuxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("CONCAT M3U format"),
    sizeof(PlaylistContext),
    m3u_probe,
    m3u_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    NULL, //flags
    NULL, //extensions
    NULL, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_m3u_tags, 0},
    ff_concatgen_read_seek, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
