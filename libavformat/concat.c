/*
 * Standard playlist/concatenation demuxer
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
static const AVCodecTag codec_concat_tags[] = {
    { 0, 0 },
};

static int concat_probe(AVProbeData *p)
{
    // concat demuxer should only be manually constructed in ffmpeg
    return 0;
}

static int concat_read_header(AVFormatContext *s,
                              AVFormatParameters *ap)
{
    // PlaylistD should be constructed externally
    return 0;
}


#if CONFIG_CONCAT_DEMUXER
AVInputFormat concat_demuxer = {
    "concat",
    NULL_IF_CONFIG_SMALL("CONCAT format"),
    0,
    concat_probe,
    concat_read_header,
    concatgen_read_packet,
    concatgen_read_close,
    concatgen_read_seek,
    concatgen_read_timestamp,
    NULL, //flags
    NULL, //extensions
    NULL, //value
    concatgen_read_play,
    concatgen_read_pause,
    (const AVCodecTag* const []){codec_concat_tags, 0},
    NULL, //m3u_read_seek2
    NULL, //metadata_conv
    NULL, //next
};
#endif
