/*
 * Generic functions used by playlist/concatenation demuxers
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

#ifndef AVFORMAT_CONCATGEN_H
#define AVFORMAT_CONCATGEN_H

#include "libavformat/avplaylist.h"

/** @brief Generic implementation of read_packet for concat demuxers.
 *  @details Read one packet and put it in 'pkt'. pts and flags are also
 *  set. 'av_new_stream' can be called only if the flag
 *  AVFMTCTX_NOHEADER is used.
 *  @return  0 on success, < 0 on error.
 *  When returning an error, pkt must not have been allocated
 *  or must be freed before returning
 */
int ff_concatgen_read_packet(AVFormatContext *s, AVPacket *pkt);

/** @brief Generic implementation of read_seek for concat demuxers.
  * @details Seek to a given timestamp relative to the frames in
  * stream component stream_index.
  * @param stream_index Must not be -1.
  * @param flags Selects which direction should be preferred if no exact
  * match is available.
  * @return >= 0 on success (but not necessarily the new offset)
 */
int ff_concatgen_read_seek(AVFormatContext *s,
                           int stream_index,
                           int64_t pts,
                           int flags);

/** @brief Generic implementation of read_timestamp for concat demuxers.
 *  @details Gets the next timestamp in stream[stream_index].time_base units.
 *  @return the timestamp or AV_NOPTS_VALUE if an error occurred
 */
int64_t ff_concatgen_read_timestamp(AVFormatContext *s,
                                    int stream_index,
                                    int64_t *pos,
                                    int64_t pos_limit);

/** @brief Generic implementation of read_close for concat demuxers.
 *  @details Close the stream. The AVFormatContext and AVStreams are not
 *  freed by this function
 */
int ff_concatgen_read_close(AVFormatContext *s);

/** @brief Generic implementation of read_play for concat demuxers.
 *  @details Starts/resumes playing - only meaningful if using a network-based
 *  format (RTSP).
 */
int ff_concatgen_read_play(AVFormatContext *s);

/** @brief Generic implementation of read_pause for concat demuxers.
 *  @details Pauses playing - only meaningful if using a network-based
 *  format (RTSP).
 */
int ff_concatgen_read_pause(AVFormatContext *s);

#endif /* AVFORMAT_CONCATGEN_H */

