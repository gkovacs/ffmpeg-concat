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

/** @file libavformat/playlist.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (PlaylistContext) and their individual playlist elements (PlayElem), each
 *  of which encapsulates its own AVFormatContext. This abstraction is used for
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
} PlayElem;

/** @struct PlaylistContext
 *  @brief Represents the playlist and contains PlayElem for each playlist item.
 */
typedef struct PlaylistContext {
    PlayElem **pelist; /**< List of PlayElem, with each representing a playlist item */
    int pelist_size; /**< Number of PlayElem stored in pelist */
    int pe_curidx; /**< Index of the PlayElem that packets are being read from */
    int64_t time_offset; /**< Time offset, in 10^-6 seconds, for all multimedia streams */
} PlaylistContext;

/** @fn int ff_playlist_init_playelem(PlayElem* pe)
 *  @brief Opens file, codecs, and streams associated with PlayElem.
 *  @param pe PlayElem to open. It should already be allocated.
 */
void ff_playlist_init_playelem(PlayElem* pe);

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

/** @fn void ff_playlist_relative_paths(char **flist, int len, const char *workingdir)
 *  @brief Converts a list of mixed relative or absolute paths into all absolute paths.
 *  @param flist List of null-terminated strings of relative or absolute paths.
 *  @param len Number of paths in flist.
 *  @param workingdir Path that strings in flist are relative to.
 */
void ff_playlist_relative_paths(char **flist, int len, const char *workingdir);

/** @fn void ff_playlist_split_encodedstring(char *s, char sep, char ***flist_ptr, int *len_ptr)
 *  @brief Splits a character-delimited string into a list of strings.
 *  @param s The input character-delimited string ("one,two,three").
 *  @param sep The delimiter character (',').
 *  @param flist_ptr Pointer to string list which will be allocated by function.
 *  @param len_ptr Number of segments the string was split into.
 */
void ff_playlist_split_encodedstring(char *s, char sep, char ***flist_ptr, int *len_ptr);

/** @fn PlaylistContext *ff_playlist_from_encodedstring(char *s, char sep)
 *  @brief Allocates and returns a PlaylistContext with playlist elements specified by a character-delimited string.
 *  @param s The input character-delimited string ("one,two,three").
 *  @param sep The delimiter character (',').
 *  @return Returns the allocated PlaylistContext.
 */
PlaylistContext *ff_playlist_from_encodedstring(char *s, char sep);

/** @fn void ff_playlist_add_path(PlaylistContext *ctx, char *itempath)
 *  @brief Adds PlayElem for item located at specified path to a PlaylistContext.
 *  @param ctx Pre-allocated PlaylistContext to add elements to.
 *  @param Absolute path to item for which to add a playlist element.
 */
void ff_playlist_add_path(PlaylistContext *ctx, char *itempath);

#endif /* AVFORMAT_PLAYLIST_H */
