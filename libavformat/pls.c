/*
 * PLS muxer and demuxer
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
#include "datanode.h"

/* The ffmpeg codecs we support, and the IDs they have in the file */
static const AVCodecTag codec_pls_tags[] = {
    { 0, 0 },
};

static int pls_probe(AVProbeData *p)
{
    if (p->buf != 0) {
        if (!strncmp(p->buf, "[playli", 7))
            return AVPROBE_SCORE_MAX;
        else
            return 0;
    }
    if (match_ext(p->filename, "pls"))
        return AVPROBE_SCORE_MAX/2;
    else
        return 0;
}

static int pls_list_files(ByteIOContext *s,
                          char ***flist_ptr,
                          unsigned int *lfx_ptr,
                          char *workingdir)
{
    StringList *l;
    DataNode *d;
    l = ff_stringlist_alloc();
    d = ff_datanode_tree_from_ini(s);
    ff_datanode_visualize(d);
    ff_datanode_filter_values_by_name(d, l, "File");
    ff_stringlist_print(l);
    ff_stringlist_export(l, flist_ptr, lfx_ptr);
    ff_playlist_relative_paths(*flist_ptr, workingdir);
    return 0;
}

static int pls_read_header(AVFormatContext *s,
                           AVFormatParameters *ap)
{
    int i;
    PlaylistContext *ctx = ff_playlist_make_context(s->filename);
    pls_list_files(s->pb,
                   &(ctx->flist),
                   &(ctx->pelist_size),
                   ctx->workingdir);
    ctx->pelist = av_malloc(ctx->pelist_size * sizeof(*(ctx->pelist)));
    memset(ctx->pelist, 0, ctx->pelist_size * sizeof(*(ctx->pelist)));
    s->priv_data = ctx;
    for (i = 0; i < ctx->pe_curidxs_size; ++i) {
        ff_playlist_populate_context(ctx, s, i);
    }
    return 0;
}

#if CONFIG_PLS_DEMUXER
AVInputFormat pls_demuxer = {
    "pls",
    NULL_IF_CONFIG_SMALL("PLS format"),
    sizeof(PlaylistContext),
    pls_probe,
    pls_read_header,
    ff_concatgen_read_packet,
    ff_concatgen_read_close,
    ff_concatgen_read_seek,
    ff_concatgen_read_timestamp,
    NULL, //flags
    NULL, //extensions
    NULL, //value
    ff_concatgen_read_play,
    ff_concatgen_read_pause,
    (const AVCodecTag* const []){codec_pls_tags, 0},
    ff_concatgen_read_seek, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
