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

/** @file playlist.h
 *  @brief General components used by playlist formats
 *  @author Geza Kovacs ( gkovacs mit edu )
 *  @details These functions are used to initialize and manipulate playlists
 *  (PlaylistContext) and their individual playlist elements (PlayElem), each
 *  of which encapsulates its own AVFormatContext. This abstraction used for
 *  implementing file concatenation and support for playlist formats.
 */

#ifndef AVFORMAT_PLAYLIST_H
#define AVFORMAT_PLAYLIST_H

#include "avformat.h"
#include "riff.h"
#include <libgen.h>

/** @struct PlayElem
 *  @brief Represents each input file on a playlist.
 */
typedef struct PlayElem {
    AVFormatContext *ic; /**< AVFormatContext for this playlist item */
    char *filename; /**< Filename with absolute path of this playlist item */
    AVInputFormat *fmt; /**< AVInputFormat manually specified for this playlist item */
    AVFormatParameters *ap; /**< AVFormatParameters for this playlist item */
} PlayElem;

/** @struct PlaylistContext
 *  @brief Represents the playlist and contains PlayElem for each playlist item.
 */
typedef struct PlaylistContext {
    PlayElem **pelist; /**< List of PlayElem, with each representing a playlist item */
    int pelist_size; /**< Length of the pelist array (number of playlist items) */
    int pe_curidx; /**< Index of the PlayElem that packets are being read from */
    char *workingdir; /**< Directory in which the playlist file is stored in */
    char *filename; /**< Filename (not path) of the playlist file */
    int time_offsets_size; /**< Number of time offsets (number of multimedia streams), 2 with audio and video. */
    int64_t *time_offsets; /**< Time offsets, in 10^-6 seconds, for each multimedia stream */
} PlaylistContext;

/** @fn int ff_playlist_init_playelem(PlayElem* pe)
 *  @brief Opens file, codecs, and streams associated with PlayElem.
 *  @param pe PlayElem to open. It should already be allocated.
 */
void ff_playlist_init_playelem(PlayElem* pe);

/** @fn PlaylistContext* ff_playlist_alloc_context(void)
 *  @brief Allocates and returns a PlaylistContext.
 *  @return Returns the allocated PlaylistContext.
 */
PlaylistContext* ff_playlist_alloc_context(void);

/** @fn void ff_playlist_populate_context(PlaylistContext *playlc, AVFormatContext *s, int stream_index)
 *  @brief Opens the current PlayElem from the PlaylistContext.
 *  @param s AVFormatContext of the concat-type demuxer, which contains the PlaylistContext.
 */
void ff_playlist_populate_context(AVFormatContext *s);

/** @fn PlaylistContext* ff_playlist_get_context(AVFormatContext *ic)
 *  @brief Returns PlaylistContext continaed within a concat-type demuxer.
 *  @param ic AVFormatContext of the concat-type demuxer, which contains the PlaylistContext.
 *  @return Returnes NULL if failed (not concat-type demuxer or Playlist not yet allocated), or PlaylistContext if succeeded.
 */
PlaylistContext* ff_playlist_get_context(AVFormatContext *ic);

/** @fn void ff_playlist_set_context(AVFormatContext *ic, PlaylistContext *ctx)
 *  @brief Sets PlaylistContext for a concat-type demuxer.
 *  @param ic AVFormatContext of the concat-type demuxer.
 *  @param ctx PlaylistContext that will be set in the concat-type demuxer.
 */
void ff_playlist_set_context(AVFormatContext *ic, PlaylistContext *ctx);

/** @fn AVStream *ff_playlist_get_stream(PlaylistContext *ctx, int pe_idx, int stream_index)
 *  @brief Obtains a specified stream from a specified item in a PlaylistContext.
 *  @param ctx PlaylistContext which contains the desired stream.
 *  @param pe_idx Index that the PlayElem has in the PlaylistContext (playlist item number), PlayElem should already be open.
 *  @param stream_index Index of the multimedia stream (audio or video) within the PlayElem.
 */
AVStream *ff_playlist_get_stream(PlaylistContext *ctx, int pe_idx, int stream_index);

#endif /* AVFORMAT_PLAYLIST_H */
