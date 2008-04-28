/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2008, Nokia
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

#ifndef __TRACKER_TYPE_UTILS_H__
#define __TRACKER_TYPE_UTILS_H__

gchar *  tracker_format_date                (const gchar  *time_string);
time_t   tracker_str_to_date                (const gchar  *time_string);
gchar *  tracker_date_to_str                (time_t        date_time);
gchar *  tracker_long_to_str                (glong         i);
gchar *  tracker_int_to_str                 (gint          i);
gchar *  tracker_uint_to_str                (guint         i);
gchar *  tracker_gint32_to_str              (gint32        i);
gchar *  tracker_guint32_to_str             (guint32       i);
gboolean tracker_str_to_uint                (const gchar  *s,
					     guint        *ret);
gint     tracker_str_in_array               (const gchar  *str,
					     gchar       **strv);
GSList * tracker_string_list_to_gslist      (const gchar **strv);
gchar ** tracker_gslist_to_string_list      (GSList       *list);
gchar *  tracker_array_to_str               (gchar       **strv,
					     gint          length,
					     gchar         sep);
gchar ** tracker_make_array_null_terminated (gchar       **strv,
					     gint          length);
void     tracker_free_array                 (gchar       **strv,
					     gint          row_count);

#endif /* __TRACKER_TYPE_UTILS_H__ */
