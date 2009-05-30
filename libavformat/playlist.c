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

#include "avformat.h"
#include "playlist.h"

int av_open_input_playelem(PlayElem *pe)
{
    return av_open_input_file(&(pe->ic), pe->filename, pe->fmt, pe->buf_size, pe->ap);
}


// based on decode_thread() in ffplay.c
int av_alloc_playelem(unsigned char *filename, PlayElem *pe)
{
//    AVProbeData *pd;
    AVFormatContext *ic;
    AVFormatParameters *ap;
//    pd = av_malloc(sizeof(AVProbeData));
//    ic = av_malloc(sizeof(AVFormatContext));
    ap = av_malloc(sizeof(AVFormatParameters));
    memset(ap, 0, sizeof(AVFormatParameters));
    ap->width = 0;
    ap->height = 0;
    ap->time_base = (AVRational){1, 25};
    ap->pix_fmt = 0;
//    pd->filename = filename;
//    pd->buf = NULL;
//    pd->buf_size = 0;
    pe->ic = ic;
    pe->filename = filename;
    pe->fmt = 0;
    pe->buf_size = 0;
    pe->ap = ap;
//    pe->fmt = pe->ic->iformat;
    return 0;
}

PlayElem* av_make_playelem(unsigned char *filename)
{
    int err;
    PlayElem *pe = av_malloc(sizeof(PlayElem));
    err = av_alloc_playelem(filename, pe);
    if (err < 0)
        print_error("during-av_alloc_playelem", err);
    err = av_open_input_playelem(pe);
    if (err < 0)
        print_error("during-open_input_playelem", err);
    pe->fmt = pe->ic->iformat;
    if (!pe->fmt)
    {
        fprintf(stderr, "pefmt not set\n");
        fflush(stderr);
    }
    err = av_find_stream_info(pe->ic);
    if (err < 0)
    {
        fprintf(stderr, "failed codec probe av_find_stream_info");
        fflush(stderr);
    }
    if(pe->ic->pb)
    {
        pe->ic->pb->eof_reached = 0;
    }
    else
    {
        fprintf(stderr, "failed pe ic pb not set");
        fflush(stderr);
    }
    if(!pe->fmt)
    {
        fprintf(stderr, "failed pe ic fmt not set");
        fflush(stderr);
    }
    return pe;
}

PlaylistD* av_make_playlistd(unsigned char **flist, int flist_len)
{
    int i;
    PlaylistD *playld = av_malloc(sizeof(PlaylistD));
    playld->pe_curidx = 0;
    playld->pelist_size = flist_len;
    playld->pelist = av_malloc(playld->pelist_size * sizeof(PlayElem*));
    memset(playld->pelist, 0, playld->pelist_size * sizeof(PlayElem*));
    for (int i = 0; i < playld->pelist_size; ++i)
    {
        playld->pelist[i] = av_make_playelem(flist[i]);
    }
    return playld;
}

int playlist_populate_context(PlaylistD *playld, AVFormatContext *s)
{
    int i;
    AVFormatContext *ic = playld->pelist[playld->pe_curidx]->ic;
    AVFormatParameters *nap = playld->pelist[playld->pe_curidx]->ap;
    ic->iformat->read_header(ic, nap);
    s->nb_streams = ic->nb_streams;
    for (i = 0; i < ic->nb_streams; ++i)
    {
        s->streams[i] = ic->streams[i];
    }
    return 0;
}

int check_file_extn(char *cch, char *extn)
{
    int pos;
    int extnl;
    pos = -1;
    extnl = 0;
    while (extn[extnl] != 0)
       ++extnl;
    pos = -1;
    if (!cch)
    {
        return 0;
    }
    if (*cch == 0)
    {
        return 0;
    }
    while (*cch != 0)
    {
        if (*cch == '.')
        {
            pos = 0;
        }
        else if (pos >= 0)
        {
            if (*cch == extn[pos])
                ++pos;
            else
                pos = -1;
            if (pos == extnl)
            {
                return 1;
            }
        }
        ++cch;
    }
    return 0;
}

