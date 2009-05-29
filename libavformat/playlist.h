/*
 * M3U muxer and demuxer
 * Copyright (c) 2001 Geza Kovacs
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

/*
 * Based on AU muxer and demuxer in au.c
 */


typedef struct PlayElem {
    AVFormatContext *ic;
    char *filename;
    AVInputFormat *fmt;
    int buf_size;
    AVFormatParameters *ap;
} PlayElem;

typedef struct PlaylistD {
    PlayElem **pelist;
    int pelist_size;
    int pe_curidx;
} PlaylistD;

int av_open_input_playelem(PlayElem *pe);

PlayElem* av_make_playelem(unsigned char *filename);

PlaylistD* av_make_playlistd(unsigned char **flist, int flist_len);

int check_file_extn(char *cch, char *extn);

int playlist_populate_context(PlaylistD *playld, AVFormatContext *s);

