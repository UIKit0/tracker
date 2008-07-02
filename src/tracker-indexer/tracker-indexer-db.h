/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2008, Nokia (urho.konttori@nokia.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA.
 */

#ifndef __TRACKER_INDEXER_DB_H__
#define __TRACKER_INDEXER_DB_H__

#include <libtracker-common/tracker-ontology.h>
#include <libtracker-db/tracker-db-interface.h>

G_BEGIN_DECLS

guint32  tracker_db_get_new_service_id (TrackerDBInterface *iface);
guint32  tracker_db_get_new_event_id   (TrackerDBInterface *iface);
void     tracker_db_increment_stats    (TrackerDBInterface *iface,
                                        TrackerService     *service);
gboolean tracker_db_create_service     (TrackerDBInterface *iface,
                                        guint32             id,
                                        TrackerService     *service,
                                        const gchar        *path,
                                        GHashTable         *metadata);
void     tracker_db_set_metadata       (TrackerDBInterface *iface,
                                        guint32             id,
                                        TrackerField       *field,
                                        const gchar        *value,
					const gchar        *parsed_value);
void     tracker_db_set_text           (TrackerDBInterface *iface,
					guint32             id,
					const gchar        *text);
gboolean tracker_db_create_event       (TrackerDBInterface *iface,
                                        guint32             id,
                                        guint32             service_id,
                                        const gchar        *type);
void     tracker_db_inc_service_id      (TrackerDBInterface *iface, 
					 guint32 id);
void     tracker_db_inc_event_id        (TrackerDBInterface *iface, 
					 guint32 id);


G_END_DECLS

#endif /* __TRACKER_INDEXER_DB_H__ */
