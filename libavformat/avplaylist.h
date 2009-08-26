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

/** @file libavformat/avplaylist.h
 *  @author Geza Kovacs ( gkovacs mit edu )
 *
 *  @brief General components used by playlist formats
 *
 *  @details These functions are used to initialize and manipulate playlists
 *  (AVPlaylistContext) and AVFormatContext for each playlist item.
 *  This abstraction is used to implement file concatenation and
 *  support for playlist formats.
 */

#ifndef AVFORMAT_AVPLAYLIST_H
#define AVFORMAT_AVPLAYLIST_H

#include <libgen.h>
#include "avformat.h"

/** @struct AVPlaylistContext
 *  @brief Represents the playlist and contains AVFormatContext for each playlist item.
 */
typedef struct AVPlaylistContext {
    char **flist;                          /**< List of file names for each playlist item */
    AVFormatContext **formatcontext_list;  /**< List of AVFormatContext for each playlist item */
    int pelist_size;                       /**< Number of playlist elements stored in formatcontext_list */
    int pe_curidx;                         /**< Index of the AVFormatContext in formatcontext_list that packets are being read from */
    int64_t *durations;                    /**< Sum of current and previous durations, in AV_TIME_BASE units, for each playlist item */
    unsigned int *nb_streams_list;         /**< Sum of current and previous number of streams in each playlist item*/
    AVFormatContext *master_formatcontext; /**< Parent AVFormatContext of which priv_data is this playlist. NULL if playlist is used standalone. */
} AVPlaylistContext;

/** @brief Allocates and sets default values for an empty AVPlaylistContext
 *  @return NULL upon failure, newly allocated, empty AVPlaylistContext upon success.
 */
AVPlaylistContext *av_playlist_alloc(void);

/** @brief Creates and adds AVFormatContext for item located at specified path to a AVPlaylistContext
 *  at specified index. Existing items will be shifted up in the list.
 *  @param ctx Pre-allocated AVPlaylistContext to add elements to.
 *  @param itempath Absolute path to item for which to add a playlist element.
 *  @param pos Zero-based index which the newly inserted item will occupy.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_insert_item(AVPlaylistContext *ctx, const char *itempath, int pos);

/** @brief Creates and adds AVFormatContext for all items in another playlist,
 *  starting at the specified index and appending until the end of the other
 *  playlist. Existing items will be shifted up in the list.
 *  @param ctx Pre-allocated AVPlaylistContext to add elements to.
 *  @param insert_ctx Pre-allocated AVPlaylistContext from which to add items from.
 *  @param pos Zero-based index which the newly inserted item will occupy.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_insert_playlist(AVPlaylistContext *ctx, AVPlaylistContext *insert_ctx, int pos);

/** @brief Removes AVFormatContext for item located at speified index from AVPlaylistContext.
 *  Existing items will be shifted down in the list.
 *  @param ctx Pre-allocated AVPlaylistContext to remove elements from.
 *  @param pos Zero-based index of the item to remove.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_remove_item(AVPlaylistContext *ctx, int pos);

/** @brief Removes all items from playlist and frees it.
 *  @param ctx Pre-allocated AVPlaylistContext to close.
 *  @return Returns 0 upon success, or negative upon failure.
 */
int av_playlist_close(AVPlaylistContext *ctx);

#endif /* AVFORMAT_AVPLAYLIST_H */
