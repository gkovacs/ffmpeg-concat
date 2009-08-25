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

#include "concatgen.h"
#include "riff.h"
#include "libavutil/avstring.h"
#include "internal.h"

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_m3u_tags[] = {
    { 0, 0 },
};

static int m3u_probe(AVProbeData *p)
{
    if (!strncmp(p->buf, "#EXTM3U", 7))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int m3u_list_files(ByteIOContext *s, char ***flist_ptr, int *len_ptr)
{
    char **flist, **flist_tmp;
    int i, bufsize;
    flist = NULL;
    i = bufsize = 0;
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
        flist_tmp = av_fast_realloc(flist, &bufsize, i+2);
        if (!flist_tmp) {
            av_log(NULL, AV_LOG_ERROR, "av_realloc error in m3u_list_files\n");
            av_free(flist);
            return AVERROR_NOMEM;
        } else
            flist = flist_tmp;
        flist[i] = av_malloc(q-linebuf+1);
        av_strlcpy(flist[i], linebuf, q-linebuf+1);
        flist[i++][q-linebuf] = 0;
    }
    *flist_ptr = flist;
    *len_ptr = i;
    if (!flist) // no files have been found
        return AVERROR_EOF;
    flist[i] = 0;
    return 0;
}

static int m3u_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    AVPlaylistContext *ctx;
    char **flist;
    int flist_len;
    m3u_list_files(s->pb, &flist, &flist_len);
    if (!flist || flist_len <= 0) {
        fprintf(stderr, "no playlist items found in %s\n", s->filename);
        return AVERROR_EOF;
    }
    av_playlist_relative_paths(flist, flist_len, dirname(s->filename));
    ctx = av_mallocz(sizeof(*ctx));
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate AVPlaylistContext in m3u_read_header\n");
        return AVERROR_NOMEM;
    }
    av_playlist_add_filelist(ctx, flist, flist_len);
    av_free(flist);
    s->priv_data = ctx;
    av_playlist_populate_context(ctx, ctx->pe_curidx);
    av_playlist_set_streams(s);
    return 0;
}

AVInputFormat m3u_demuxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("CONCAT M3U format"),
    sizeof(AVPlaylistContext),
    m3u_probe,
    m3u_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    0, //flags
    NULL, //extensions
    0, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_m3u_tags, 0},
};
