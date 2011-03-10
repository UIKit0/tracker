/*
 * Copyright (C) 2011, Nokia <ivan.frade@nokia.com>
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

#ifndef __TRACKER_EXTRACT_MODULE_MANAGER_H__
#define __TRACKER_EXTRACT_MODULE_MANAGER_H__

#include <glib.h>
#include <gmodule.h>
#include <libtracker-sparql/tracker-sparql.h>

G_BEGIN_DECLS

#define __LIBTRACKER_EXTRACT_INSIDE__

typedef gboolean (* TrackerExtractMetadataFunc) (const gchar          *uri,
                                                 const gchar          *mime_type,
                                                 TrackerSparqlBuilder *preupdate,
                                                 TrackerSparqlBuilder *metadata);


gboolean  tracker_extract_module_manager_init                (void) G_GNUC_CONST;
GModule * tracker_extract_module_manager_get_for_mimetype    (const gchar                *mimetype,
                                                              TrackerExtractMetadataFunc *func);
gboolean  tracker_extract_module_manager_mimetype_is_handled (const gchar *mimetype);

#undef __LIBTRACKER_EXTRACT_INSIDE__

G_END_DECLS

#endif /* __TRACKER_EXTRACT_MODULE_MANAGER_H__ */
