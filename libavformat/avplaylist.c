/*
 * General components used by playlist formats
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

/** @file libavformat/avplaylist.c
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (AVPlaylistContext) and AVFormatContext for each playlist item.
 *  This abstraction is used to implement file concatenation and
 *  support for playlist formats.
 */

#include "avplaylist.h"
#include "riff.h"
#include "libavutil/avstring.h"
#include "internal.h"

int av_playlist_split_encodedstring(const char *s,
                                    const char sep,
                                    char ***flist_ptr,
                                    int *len_ptr)
{
    char c, *ts, **flist;
    int i, len, buflen, *sepidx, *sepidx_tmp;
    sepidx = NULL;
    buflen = len = 0;
    sepidx_tmp = av_fast_realloc(sepidx, &buflen, ++len);
    if (!sepidx_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_split_encodedstring\n");
        av_free(sepidx);
        return AVERROR_NOMEM;
    }
    else
        sepidx = sepidx_tmp;
    sepidx[0] = 0;
    ts = s;
    while ((c = *ts++) != 0) {
        if (c == sep) {
            sepidx[len] = ts-s;
            sepidx_tmp = av_fast_realloc(sepidx, &buflen, ++len);
            if (!sepidx_tmp) {
                av_free(sepidx);
                av_log(NULL, AV_LOG_ERROR, "av_fast_realloc error in av_playlist_split_encodedstring\n");
                *flist_ptr = NULL;
                *len_ptr = 0;
                return AVERROR_NOMEM;
            } else
                sepidx = sepidx_tmp;
        }
    }
    sepidx[len] = ts-s;
    ts = s;
    *len_ptr = len;
    *flist_ptr = flist = av_malloc(sizeof(*flist) * (len+1));
    flist[len] = 0;
    for (i = 0; i < len; ++i) {
        flist[i] = av_malloc(sepidx[i+1]-sepidx[i]);
        if (!flist[i]) {
            av_log(NULL, AV_LOG_ERROR, "av_malloc error in av_playlist_split_encodedstring\n");
            *flist_ptr = NULL;
            *len_ptr = 0;
            return AVERROR_NOMEM;
        }
        av_strlcpy(flist[i], ts+sepidx[i], sepidx[i+1]-sepidx[i]);
    }
    av_free(sepidx);
}

int av_playlist_insert_item(AVPlaylistContext *ctx, const char *itempath, int pos)
{
    int i;
    int64_t *durations_tmp;
    unsigned int *nb_streams_list_tmp;
    char **flist_tmp;
    flist_tmp = av_realloc(ctx->flist, sizeof(*(ctx->flist)) * (++ctx->pelist_size+1));
    if (!flist_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_insert_item\n");
        av_free(ctx->flist);
        ctx->flist = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->flist = flist_tmp;
    for (i = ctx->pelist_size; i > pos; --i)
        ctx->flist[i] = ctx->flist[i - 1];
    ctx->flist[pos] = itempath;
    ctx->flist[ctx->pelist_size] = NULL;
    durations_tmp = av_realloc(ctx->durations,
                               sizeof(*(ctx->durations)) * (ctx->pelist_size+1));
    if (!durations_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_insert_item\n");
        av_free(ctx->durations);
        ctx->durations = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->durations = durations_tmp;
    for (i = ctx->pelist_size; i > pos; --i)
        ctx->durations[i] = ctx->durations[i - 1];
    ctx->durations[pos] = 0;
    ctx->durations[ctx->pelist_size] = 0;
    nb_streams_list_tmp = av_realloc(ctx->nb_streams_list,
                                     sizeof(*(ctx->nb_streams_list)) * (ctx->pelist_size+1));
    if (!nb_streams_list_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_insert_item\n");
        av_free(ctx->nb_streams_list);
        ctx->nb_streams_list = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->nb_streams_list = nb_streams_list_tmp;
    for (i = ctx->pelist_size; i > pos; --i)
        ctx->nb_streams_list[i] = ctx->nb_streams_list[i - 1];
    ctx->nb_streams_list[pos] = 0;
    ctx->nb_streams_list[ctx->pelist_size] = 0;
    return 0;
}

int av_playlist_add_item(AVPlaylistContext *ctx, const char *itempath)
{
    return av_playlist_insert_item(ctx, itempath, ctx->pelist_size);
}

int av_playlist_remove_item(AVPlaylistContext *ctx, int pos)
{
    int i;
    int64_t *durations_tmp;
    unsigned int *nb_streams_list_tmp;
    AVFormatContext **formatcontext_list_tmp;
    char **flist_tmp;
    if (pos >= ctx->pelist_size || !ctx->flist || !ctx->durations || !ctx->nb_streams_list)
        return AVERROR_INVALIDDATA;
    for (i = pos; i < ctx->pelist_size; ++i)
        ctx->flist[i] = ctx->flist[i + 1];
    flist_tmp = av_realloc(ctx->flist, sizeof(*(ctx->flist)) * (--ctx->pelist_size+1));
    if (!flist_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_remove_item\n");
        av_free(ctx->flist);
        ctx->flist = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->flist = flist_tmp;
    ctx->flist[ctx->pelist_size] = NULL;
    for (i = pos; i < ctx->pelist_size; ++i)
        ctx->durations[i] = ctx->durations[i + 1];
    durations_tmp = av_realloc(ctx->durations,
                               sizeof(*(ctx->durations)) * (ctx->pelist_size+1));
    if (!durations_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_remove_item\n");
        av_free(ctx->durations);
        ctx->durations = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->durations = durations_tmp;
    ctx->durations[ctx->pelist_size] = 0;
    for (i = pos; i < ctx->pelist_size; ++i)
        ctx->nb_streams_list[i] = ctx->nb_streams_list[i + 1];
    nb_streams_list_tmp = av_realloc(ctx->nb_streams_list,
                                     sizeof(*(ctx->nb_streams_list)) * (ctx->pelist_size+1));
    if (!nb_streams_list_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_remove_item\n");
        av_free(ctx->nb_streams_list);
        ctx->nb_streams_list = NULL;
        return AVERROR_NOMEM;
    } else
        ctx->nb_streams_list = nb_streams_list_tmp;
    ctx->nb_streams_list[ctx->pelist_size] = 0;
    if (ctx->formatcontext_list && ctx->formatcontext_list[pos]) {
        av_close_input_file(ctx->formatcontext_list[pos]);
        av_close_input_stream(ctx->formatcontext_list[pos]);
        av_free(ctx->formatcontext_list[pos]);
        ctx->formatcontext_list[pos] = NULL;
    }
    for (i = pos; i < ctx->pelist_size; ++i)
        ctx->formatcontext_list[i] = ctx->formatcontext_list[i + 1];
    formatcontext_list_tmp = av_realloc(ctx->formatcontext_list,
                                        sizeof(*(ctx->formatcontext_list)) * (ctx->pelist_size+1));
    if (!formatcontext_list_tmp) {
        av_log(NULL, AV_LOG_ERROR, "av_realloc error in av_playlist_remove_item\n");
        av_free(ctx->formatcontext_list);
        return AVERROR_NOMEM;
    } else
        ctx->formatcontext_list = formatcontext_list_tmp;
    ctx->formatcontext_list[ctx->pelist_size] = NULL;
    return 0;
}

void av_playlist_relative_paths(char **flist,
                                int len,
                                const char *workingdir)
{
    int i;
    for (i = 0; i < len; ++i) { // determine if relative paths
        char *full_file_path;
        int workingdir_len, filename_len;
        workingdir_len = strlen(workingdir);
        filename_len = strlen(flist[i]);
        full_file_path = av_malloc(workingdir_len + filename_len + 2);
        av_strlcpy(full_file_path, workingdir, workingdir_len + 1);
        full_file_path[workingdir_len] = '/';
        full_file_path[workingdir_len + 1] = 0;
        av_strlcat(full_file_path, flist[i], workingdir_len + filename_len + 2);
        if (url_exist(full_file_path))
            flist[i] = full_file_path;
    }
}

int av_playlist_close(AVPlaylistContext *ctx)
{
    int i, err;
    while (ctx->pelist_size > 0) {
        err = av_playlist_remove_item(ctx->pelist_size-1);
        if (err) {
            av_log(NULL, AV_LOG_ERROR, "failed to remove item %d from playlist", ctx->pelist_size-1);
            return err;
        }
    }
    if (ctx->flist)
        av_free(ctx->flist);
    if (ctx->durations)
        av_free(ctx->durations);
    if (ctx->nb_streams_list)
        av_free(ctx->nb_streams_list);
    if (ctx->formatcontext_list)
        av_free(ctx->formatcontext_list);
    av_free(ctx);
    return 0;
}
