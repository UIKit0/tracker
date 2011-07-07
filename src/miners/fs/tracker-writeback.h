/*
 * Copyright (C) 2008, Nokia <ivan.frade@nokia.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __TRACKER_WRITEBACK_H__
#define __TRACKER_WRITEBACK_H__


#include <libtracker-miner/tracker-miner.h>

#include "tracker-config.h"

#include "tracker-miner-files.h"

G_BEGIN_DECLS

void tracker_writeback_init     (TrackerMinerFiles  *miner_files,
                                 GError            **error);
void tracker_writeback_shutdown (void);

G_END_DECLS

#endif /* __TRACKER_WRITEBACK_H__ */
