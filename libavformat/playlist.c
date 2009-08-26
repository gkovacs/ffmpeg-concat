/*
 * Internal functions used to manipulate playlists
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

/** @file libavformat/playlist.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief Internal functions used to manipulate playlists
 *
 *  @details These functions are used internally for manipulating playlists.
 *  The public playlist API can be found in avplaylist.h
 */

#include "playlist.h"

int ff_playlist_populate_context(AVPlaylistContext *ctx, int pe_curidx)
{
    AVFormatContext **formatcontext_list_tmp = av_realloc(ctx->formatcontext_list, sizeof(*(ctx->formatcontext_list)) * (pe_curidx+2));
    if (!formatcontext_list_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_populate_context\n");
        av_free(ctx->formatcontext_list);
        return AVERROR_NOMEM;
    } else
        ctx->formatcontext_list = formatcontext_list_tmp;
    ctx->formatcontext_list[pe_curidx+1] = NULL;
    if (!(ctx->formatcontext_list[pe_curidx] = av_playlist_alloc_formatcontext(ctx->flist[pe_curidx])))
        return AVERROR_NOFMT;
    ctx->nb_streams_list[pe_curidx] = ctx->formatcontext_list[pe_curidx]->nb_streams;
    if (pe_curidx > 0)
        ctx->durations[pe_curidx] = ctx->durations[pe_curidx - 1] + ctx->formatcontext_list[pe_curidx]->duration;
    else
        ctx->durations[pe_curidx] = 0;
    return 0;
}
