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
 *  (AVPlaylistContext) and their individual playlist elements (PlayElem), each
 *  of which encapsulates its own AVFormatContext. This abstraction is used for
 *  implementing file concatenation and support for playlist formats.
 */

#include "libavformat/avplaylist.h"
#include "riff.h"
#include "libavutil/avstring.h"
#include "internal.h"
#include "concat.h"

#define STREAM_CACHE_SIZE (6)

AVFormatContext *av_playlist_alloc_formatcontext(char *filename)
{
    int err;
    AVFormatContext *ic = avformat_alloc_context();
    err = av_open_input_file(&ic, filename, ic->iformat, 0, NULL);
    if (err < 0)
        av_log(ic, AV_LOG_ERROR, "Error during av_open_input_file\n");
    err = av_find_stream_info(ic);
    if (err < 0)
        av_log(ic, AV_LOG_ERROR, "Could not find stream info\n");
    return ic;
}

void av_playlist_populate_context(AVPlaylistContext *ctx, int pe_curidx)
{
    ctx->icl = av_realloc(ctx->icl, sizeof(*(ctx->icl)) * (pe_curidx+2));
    ctx->icl[pe_curidx+1] = NULL;
    ctx->icl[pe_curidx] = av_playlist_alloc_formatcontext(ctx->flist[pe_curidx]);
    ctx->nb_streams_list[pe_curidx] = ctx->icl[pe_curidx]->nb_streams;
}

void av_playlist_set_streams(AVFormatContext *s)
{
    int i;
    AVPlaylistContext *ctx = s->priv_data;
    AVFormatContext *ic = ctx->icl[ctx->pe_curidx];
    int offset = av_playlist_streams_offset_from_playidx(ctx, ctx->pe_curidx);
    ic->iformat->read_header(ic, NULL);
    for (i = 0; i < ic->nb_streams; ++i) {
        s->streams[offset + i] = ic->streams[i];
        ic->streams[i]->index += offset;
        if (!ic->streams[i]->codec->codec) {
            AVCodec *codec = avcodec_find_decoder(ic->streams[i]->codec->codec_id);
            if (!codec) {
                av_log(ic->streams[i]->codec,
                       AV_LOG_ERROR,
                       "Decoder (codec id %d) not found for input stream #%d\n",
                       ic->streams[i]->codec->codec_id,
                       ic->streams[i]->index);
                return;
            }
            if (avcodec_open(ic->streams[i]->codec, codec) < 0) {
                av_log(ic->streams[i]->codec,
                       AV_LOG_ERROR,
                       "Error while opening decoder for input stream #%d\n",
                       ic->streams[i]->index);
                return;
            }
        }
    }
    s->nb_streams        = ic->nb_streams + offset;
    s->packet_buffer     = ic->packet_buffer;
    s->packet_buffer_end = ic->packet_buffer_end;
    if (ic->iformat->read_timestamp)
        s->iformat->read_timestamp = ff_concatgen_read_timestamp;
    else
        s->iformat->read_timestamp = NULL;
}

AVPlaylistContext *av_playlist_get_context(AVFormatContext *ic)
{
    if (ic && ic->iformat && ic->iformat->long_name && ic->priv_data &&
        !strncmp(ic->iformat->long_name, "CONCAT", 6))
        return ic->priv_data;
    else
        return NULL;
}

AVFormatContext *av_playlist_formatcontext_from_filelist(const char **flist, int len)
{
    AVPlaylistContext *ctx;
    AVFormatContext *ic;
    ctx = av_playlist_from_filelist(flist, len);
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "failed to create AVPlaylistContext in av_playlist_formatcontext_from_filelist\n");
        return NULL;
    }
    ic = avformat_alloc_context();
    ic->iformat = ff_concat_alloc_demuxer();
    ic->priv_data = ctx;
    return ic;
}

void av_playlist_split_encodedstring(const char *s,
                                     const char sep,
                                     char ***flist_ptr,
                                     int *len_ptr)
{
    char c, *ts, **flist;
    int i, len, buflen, *sepidx;
    sepidx = NULL;
    buflen = len = 0;
    sepidx = av_fast_realloc(sepidx, &buflen, ++len);
    sepidx[0] = 0;
    ts = s;
    while ((c = *ts++) != 0) {
        if (c == sep) {
            sepidx[len] = ts-s;
            sepidx = av_fast_realloc(sepidx, &buflen, ++len);
            if (!sepidx) {
                av_log(NULL, AV_LOG_ERROR, "av_fast_realloc error in av_playlist_split_encodedstring\n");
                continue;
            }
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
            continue;
        }
        av_strlcpy(flist[i], ts+sepidx[i], sepidx[i+1]-sepidx[i]);
    }
    av_free(sepidx);
}

AVPlaylistContext *av_playlist_from_filelist(const char **flist, int len)
{
    int i;
    AVPlaylistContext *ctx;
    ctx = av_mallocz(sizeof(*ctx));
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "av_mallocz error in av_playlist_from_encodedstring\n");
        return NULL;
    }
    for (i = 0; i < len; ++i)
        av_playlist_add_path(ctx, flist[i]);
    return ctx;
}

AVPlaylistContext *av_playlist_from_encodedstring(const char *s, const char sep)
{
    AVPlaylistContext *ctx;
    char **flist;
    int i, len;
    av_playlist_split_encodedstring(s, sep, &flist, &len);
    if (len <= 1) {
        for (i = 0; i < len; ++i)
            av_free(flist[i]);
        av_free(flist);
        return NULL;
    }
    ctx = av_playlist_from_filelist(flist, len);
    av_free(flist);
    return ctx;
}

void av_playlist_add_path(AVPlaylistContext *ctx, const char *itempath)
{
    ctx->flist = av_realloc(ctx->flist, sizeof(*(ctx->flist)) * (++ctx->pelist_size+1));
    ctx->flist[ctx->pelist_size] = NULL;
    ctx->flist[ctx->pelist_size-1] = itempath;
    ctx->durations = av_realloc(ctx->durations,
                                sizeof(*(ctx->durations)) * (ctx->pelist_size+1));
    ctx->durations[ctx->pelist_size] = 0;
    ctx->nb_streams_list = av_realloc(ctx->nb_streams_list,
                                      sizeof(*(ctx->nb_streams_list)) * (ctx->pelist_size+1));
    ctx->nb_streams_list[ctx->pelist_size] = 0;
}

void av_playlist_relative_paths(char **flist,
                                int len,
                                const char *workingdir)
{
    int i;
    for (i = 0; i < len; ++i) { // determine if relative paths
        char *full_file_path;
        int workingdir_len = strlen(workingdir);
        int filename_len = strlen(flist[i]);
        full_file_path = av_malloc(workingdir_len + filename_len + 2);
        av_strlcpy(full_file_path, workingdir, workingdir_len + 1);
        full_file_path[workingdir_len] = '/';
        full_file_path[workingdir_len + 1] = 0;
        av_strlcat(full_file_path, flist[i], workingdir_len + filename_len + 2);
        if (url_exist(full_file_path))
            flist[i] = full_file_path;
    }
}

int64_t av_playlist_time_offset(const int64_t *durations, int stream_index)
{
    int i, cache_num;
    int64_t total = 0;
    static int cache_stidx[STREAM_CACHE_SIZE] = {-1};
    static int64_t cache_timeoffset[STREAM_CACHE_SIZE] = {-1};
    for (i = 0; i < STREAM_CACHE_SIZE; ++i) {
        if (cache_stidx[i] == stream_index)
            return cache_timeoffset[i];
    }
    cache_num = stream_index % STREAM_CACHE_SIZE;
    cache_stidx[cache_num] = stream_index;
    for (i = 0; i < stream_index; ++i) {
        total += durations[i];
    }
    return (cache_timeoffset[cache_num] = total);
}

int av_playlist_stream_index_from_time(AVPlaylistContext *ctx,
                                       int64_t pts,
                                       int64_t *localpts)
{
    int i;
    int64_t total;
    i = total = 0;
    while (pts >= total) {
        if (i >= ctx->pelist_size)
            break;
        total += ctx->durations[i++];
    }
    if (localpts)
        *localpts = pts-(total-ctx->durations[i-1]);
    return i;
}

int av_playlist_localstidx_from_streamidx(AVPlaylistContext *ctx, int stream_index)
{
    int i, total, cache_num;
    static int cache_globalstidx[STREAM_CACHE_SIZE] = {-1};
    static int cache_localstidx[STREAM_CACHE_SIZE] = {-1};
    for (i = 0; i < STREAM_CACHE_SIZE; ++i) {
        if (cache_globalstidx[i] == stream_index)
            return cache_localstidx[i];
    }
    i = total = 0;
    cache_num = stream_index % STREAM_CACHE_SIZE;
    cache_globalstidx[cache_num] = stream_index;
    while (stream_index >= total)
        total += ctx->nb_streams_list[i++];
    return (cache_localstidx[cache_num] = stream_index - (total - ctx->nb_streams_list[i-1]));
}

int av_playlist_streams_offset_from_playidx(AVPlaylistContext *ctx, int playidx)
{
    int i, total, cache_num;
    static int cache_playidx[STREAM_CACHE_SIZE] = {-1};
    static int cache_offset[STREAM_CACHE_SIZE] = {-1};
    for (i = 0; i < STREAM_CACHE_SIZE; ++i) {
        if (cache_playidx[i] == playidx)
            return cache_offset[i];
    }
    i = total = 0;
    cache_num = playidx % STREAM_CACHE_SIZE;
    cache_playidx[cache_num] = playidx;
    while (playidx > i)
        total += ctx->nb_streams_list[i++];
    return (cache_offset[cache_num] = total);
}

