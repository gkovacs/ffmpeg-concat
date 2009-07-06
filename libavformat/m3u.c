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

#include "concatgen.h"

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

static int m3u_list_files(ByteIOContext *s, PlaylistContext *ctx)
//                          char ***flist_ptr,
//                          unsigned int *lfx_ptr,
//                          char *workingdir)
{
    char **flist;
    int i, j;
    int bufsize = 16;
    i = 0;
    flist = av_malloc(sizeof(char*) * bufsize);
    while (1) {
        char *c = ff_buf_getline(s);
        if (c == NULL) // EOF
            break;
        if (*c == 0) // hashed out
            continue;
        flist[i] = c;
        if (++i == bufsize) {
            bufsize += 16;
            flist = av_realloc(flist, sizeof(char*) * bufsize);
        }
    }
    ctx->pelist_size = i;
    flist[i] = 0;
    ff_playlist_relative_paths(flist, ctx->workingdir);
    ctx->pelist = av_malloc(ctx->pelist_size * sizeof(*(ctx->pelist)));
    memset(ctx->pelist, 0, ctx->pelist_size * sizeof(*(ctx->pelist)));
    for (i = 0; i < ctx->pelist_size; ++i) {
        ctx->pelist[i] = av_malloc(sizeof(*(ctx->pelist[i])));
        ctx->pelist[i]->filename = flist[i];
    }
    av_free(flist);
    return 0;
}

static int m3u_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    int i;
    PlaylistContext *ctx = ff_playlist_make_context(s->filename);
    m3u_list_files(s->pb, ctx);
    s->priv_data = ctx;
    for (i = 0; i < ctx->pe_curidxs_size; ++i) {
        ff_playlist_populate_context(ctx, s, i);
    }
    return 0;
}

#if CONFIG_M3U_DEMUXER
AVInputFormat m3u_demuxer = {
    "m3u",
    NULL_IF_CONFIG_SMALL("M3U format"),
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
