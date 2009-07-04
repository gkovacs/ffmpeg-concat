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
    PlaylistContext *ctx = ff_playlist_make_context(s->filename);
    m3u_list_files(s->pb,
                   &(ctx->flist),
                   &(ctx->pelist_size),
                   ctx->workingdir);
    ctx->pelist = av_malloc(ctx->pelist_size * sizeof(PlayElem*));
    memset(ctx->pelist, 0, ctx->pelist_size * sizeof(PlayElem*));
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
    0,
    m3u_probe,
    m3u_read_header,
    concatgen_read_packet,
    concatgen_read_close,
    concatgen_read_seek,
    concatgen_read_timestamp,
    NULL, //flags
    NULL, //extensions
    NULL, //value
    concatgen_read_play,
    concatgen_read_pause,
    (const AVCodecTag* const []){codec_m3u_tags, 0},
    NULL, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
