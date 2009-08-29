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
#include "concatgen.h"
#include "concat.h"

AVFormatContext *ff_playlist_alloc_formatcontext(char *filename)
{
    int err;
    AVFormatContext *ic = avformat_alloc_context();
    if (!ic) {
        av_log(NULL, AV_LOG_ERROR, "unable to allocate AVFormatContext in ff_playlist_alloc_formatcontext\n");
        return NULL;
    }
    err = av_open_input_file(&ic, filename, ic->iformat, 0, NULL);
    if (err < 0) {
        av_log(ic, AV_LOG_ERROR, "Error during av_open_input_file\n");
        av_free(ic);
        return NULL;
    }
    err = av_find_stream_info(ic);
    if (err < 0) {
        av_log(ic, AV_LOG_ERROR, "Could not find stream info\n");
        av_close_input_file(ic);
        return NULL;
    }
    return ic;
}

AVFormatContext *ff_playlist_alloc_concat_formatcontext(void)
{
    AVFormatContext *ic;
    AVPlaylistContext *ctx = av_playlist_alloc();
    if (!ctx) {
        av_log(NULL, AV_LOG_ERROR, "failed to allocate AVPlaylistContext in ff_playlist_alloc_concat_formatcontext\n");
        return NULL;
    }
    ic = avformat_alloc_context();
    ic->iformat = ff_concat_alloc_demuxer();
    ic->priv_data = ctx;
    ctx->master_formatcontext = ic;
    return ic;
}

int ff_playlist_set_streams(AVPlaylistContext *ctx)
{
    unsigned int i;
    AVFormatContext *s, *ic;
    unsigned int offset = 0;
    if (!(s = ctx->master_formatcontext))
        return 0;
    ic = ctx->formatcontext_list[ctx->pe_curidx];
    if (ctx->pe_curidx > 0)
        offset = ctx->nb_streams_list[ctx->pe_curidx - 1];
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
                return AVERROR_NOFMT;
            }
            if (avcodec_open(ic->streams[i]->codec, codec) < 0) {
                av_log(ic->streams[i]->codec,
                       AV_LOG_ERROR,
                       "Error while opening decoder for input stream #%d\n",
                       ic->streams[i]->index);
                return AVERROR_IO;
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
    return 0;
}

int ff_playlist_split_encodedstring(const char *s,
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

void ff_playlist_relative_paths(char **flist,
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

int ff_playlist_stream_index_from_time(AVPlaylistContext *ctx,
                                       int64_t pts,
                                       int64_t *localpts)
{
    int64_t total = 0;
    int i = ctx->pe_curidx;
    while (pts >= total) {
        if (i >= ctx->pelist_size)
            break;
        total = ctx->durations[i++];
    }
    if (localpts)
        *localpts = pts-(total-ctx->durations[i-1]);
    return i;
}

int ff_playlist_localstidx_from_streamidx(AVPlaylistContext *ctx, int stream_index)
{
    unsigned int i, stream_total, stream_offset;
    stream_total = stream_offset = 0;
    for (i = 0; stream_index >= stream_total; ++i) {
        stream_offset = stream_total;
        stream_total = ctx->nb_streams_list[i];
    }
    return stream_index - stream_offset;
}
