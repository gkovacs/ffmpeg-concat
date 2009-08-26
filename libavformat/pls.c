/*
 * PLS playlist demuxer
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

/** @file libavformat/pls.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief PLS playlist demuxer
 */

#include "concatgen.h"
#include "riff.h"
#include "libavutil/avstring.h"
#include "internal.h"
#include "playlist.h"

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_pls_tags[] = {
    { 0, 0 },
};

static int pls_probe(AVProbeData *p)
{
    if (!strncmp(p->buf, "[playlist]", 10))
        return AVPROBE_SCORE_MAX;
    else
        return 0;
}

static int pls_list_files(ByteIOContext *b, char ***flist_ptr, int *len_ptr)
{
    int i, j, c;
    unsigned int buflen;
    char state;
    char **flist, **flist_tmp;
    char buf[1024];
    char buf_tag[5] = {0};
    const char match_tag[] = "\nFile";
    flist = NULL;
    state = buflen = i = j = 0;
    while ((c = url_fgetc(b))) {
        if (c == EOF)
            break;
        if (state == 0) {
            memmove(buf_tag, buf_tag+1, 4);
            buf_tag[4] = c;
            if (!memcmp(buf_tag, match_tag, 5))
                state = 1;
        } else if (state == 1) {
            if (c == '=')
                state = 2;
            else if (c == '#')
                state = 0;
        } else {
            if (c == '\n' || c == '#') {
                termfn:
                buf[i++] = 0;
                flist_tmp = av_fast_realloc(flist, &buflen, sizeof(*flist) * (j+2));
                if (!flist_tmp) {
                    av_log(NULL, AV_LOG_ERROR, "av_realloc error in m3u_list_files\n");
                    av_free(flist);
                    return AVERROR_NOMEM;
                } else
                    flist = flist_tmp;
                flist[j] = av_malloc(i);
                av_strlcpy(flist[j++], buf, i);
                i = 0;
                state = 0;
                buf_tag[sizeof(buf_tag)-1] = c;
                continue;
            } else {
                buf[i++] = c;
                if (i >= sizeof(buf)-1)
                    goto termfn;
            }
        }
    }
    *flist_ptr = flist;
    *len_ptr = j;
    if (!flist) // no files have been found
        return AVERROR_EOF;
    flist[j] = 0;
    return 0;
}

static int pls_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    AVPlaylistContext *ctx;
    char **flist;
    int flist_len, i;
    pls_list_files(s->pb, &flist, &flist_len);
    if (!flist || flist_len <= 0) {
        fprintf(stderr, "no playlist items found in %s\n", s->filename);
        return AVERROR_EOF;
    }
    av_playlist_relative_paths(flist, flist_len, dirname(s->filename));
    ctx = av_mallocz(sizeof(*ctx));
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate AVPlaylistContext in pls_read_header\n");
        return AVERROR_NOMEM;
    }
    for (i = 0; i < flist_len; ++i)
        av_playlist_add_path(ctx, flist[i]);
    av_free(flist);
    s->priv_data = ctx;
    ctx->master_formatcontext = s;
    ff_playlist_populate_context(ctx, ctx->pe_curidx);
    ff_playlist_set_streams(ctx);
    return 0;
}

AVInputFormat pls_demuxer = {
    "pls",
    NULL_IF_CONFIG_SMALL("CONCAT PLS format"),
    sizeof(AVPlaylistContext),
    pls_probe,
    pls_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    0, //flags
    NULL, //extensions
    0, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_pls_tags, 0},
};
