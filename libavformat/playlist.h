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


typedef struct PlayElem {
    AVFormatContext *ic;
    char *filename;
    AVInputFormat *fmt;
    int buf_size;
    AVFormatParameters *ap;
    int64_t time_offset;
    int64_t indv_time;
} PlayElem;

typedef struct PlaylistD {
    char **flist;
//    int flist_len;
    PlayElem **pelist;
    int pelist_size;
    int pe_curidx;
    AVChapter **chlist;
    int chlist_size;
    int ch_curidx;
    char *workingdir;
    char *filename;
} PlaylistD;

int av_open_input_playelem(PlayElem *pe);

PlayElem* av_make_playelem(unsigned char *filename);

PlaylistD* av_make_playlistd(unsigned char **flist, int flist_len);

int check_file_extn(char *cch, char *extn);

int playlist_populate_context(PlaylistD *playld, AVFormatContext *s);

char* conc_strings(char *string1, char *string2);

char* buf_getline(ByteIOContext *s);

void split_wd_fn(char *filepath, char **workingdir, char **filename);

unsigned int get_stream_offset(AVFormatContext *s);
