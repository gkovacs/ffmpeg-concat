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

#ifndef _PLAYLIST_H
#define _PLAYLIST_H

#include "avformat.h"
#include "riff.h"

typedef struct PlayElem {
    AVFormatContext *ic;
    char *filename;
    AVInputFormat *fmt;
    int buf_size;
    AVFormatParameters *ap;
    int64_t time_offset;
    int64_t indv_time;
} PlayElem;

typedef struct PlaylistContext {
    char **flist;
//    int flist_len;
    PlayElem **pelist;
    int pelist_size;
    int *pe_curidxs;
    int pe_curidxs_size;
//    int pe_curidx;
    AVChapter **chlist;
    int chlist_size;
    int ch_curidx;
    char *workingdir;
    char *filename;
    int64_t *time_offsets;
    int time_offsets_size;
} PlaylistContext;

PlayElem* ff_make_playelem(char *filename);

PlaylistContext* ff_make_playlistc(char *filename);

int ff_playlist_populate_context(PlaylistContext *playlc, AVFormatContext *s, int stream_index);

char* ff_conc_strings(char *string1, char *string2);

char* ff_buf_getline(ByteIOContext *s);

void ff_split_wd_fn(char *filepath, char **workingdir, char **filename);

int64_t ff_conv_stream_time(AVFormatContext *ic, int stream_index, int64_t avt_duration);

int64_t ff_conv_base_time(AVFormatContext *ic, int stream_index, int64_t stream_duration);

int64_t ff_get_duration(AVFormatContext *ic, int stream_index);

#endif /* _PLAYLIST_H */
