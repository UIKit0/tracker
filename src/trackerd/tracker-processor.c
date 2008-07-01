/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
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

#include "config.h"

#include <string.h>

#include <glib.h>
#include <gio/gio.h>

#include <libtracker-common/tracker-module-config.h>
#include <libtracker-common/tracker-hal.h>

#include "tracker-processor.h"
#include "tracker-crawler.h"
#include "tracker-monitor.h"

#define TRACKER_PROCESSOR_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), TRACKER_TYPE_PROCESSOR, TrackerProcessorPrivate))

typedef struct TrackerProcessorPrivate TrackerProcessorPrivate;

struct TrackerProcessorPrivate {
	TrackerConfig  *config; 
#ifdef HAVE_HAL
	TrackerHal     *hal; 
#endif  /* HAVE_HAL */
	TrackerCrawler *crawler; 
	
	GList          *modules; 
	GList          *current_module; 

	GTimer         *timer;

	gboolean        finished;
};

enum {
	FINISHED,
	LAST_SIGNAL
};

static void tracker_processor_finalize (GObject          *object);
static void process_next_module        (TrackerProcessor *processor);
static void crawler_finished_cb        (TrackerCrawler   *crawler,
					gpointer          user_data);

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (TrackerProcessor, tracker_processor, G_TYPE_OBJECT)

static void
tracker_processor_class_init (TrackerProcessorClass *class)
{
	GObjectClass *object_class = G_OBJECT_CLASS (class);

	object_class->finalize = tracker_processor_finalize;

	signals [FINISHED] = 
		g_signal_new ("finished",
			      G_OBJECT_CLASS_TYPE (object_class),
			      G_SIGNAL_RUN_LAST,
			      G_STRUCT_OFFSET (TrackerProcessorClass, finished),
			      NULL, NULL,
			      g_cclosure_marshal_VOID__VOID,
			      G_TYPE_NONE, 0);

	g_type_class_add_private (object_class, sizeof (TrackerProcessorPrivate));
}

static void
tracker_processor_init (TrackerProcessor *processor)
{
	TrackerProcessorPrivate *priv;

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

	priv->modules = tracker_module_config_get_modules ();
}

static void
tracker_processor_finalize (GObject *object)
{
	TrackerProcessorPrivate *priv;

	priv = TRACKER_PROCESSOR_GET_PRIVATE (object);

	if (priv->timer) {
		g_timer_destroy (priv->timer);
	}

	g_list_free (priv->modules);

	g_signal_handlers_disconnect_by_func (priv->crawler,
					      G_CALLBACK (crawler_finished_cb),
					      object);
	g_object_unref (priv->crawler);

#ifdef HAVE_HAL
	g_object_unref (priv->hal);
#endif /* HAVE_HAL */

	g_object_unref (priv->config);

	G_OBJECT_CLASS (tracker_processor_parent_class)->finalize (object);
}

static void
add_monitors (const gchar *name)
{
	GList *monitors;
	GList *l;

	monitors = tracker_module_config_get_monitor_directories (name);
	
	for (l = monitors; l; l = l->next) {
		GFile       *file;
		const gchar *path;

		path = l->data;

		g_message ("  Adding specific directory monitor:'%s'", path);

		file = g_file_new_for_path (path);
		tracker_monitor_add (file);
		g_object_unref (file);
	}

	g_list_free (monitors);

	if (!monitors) {
		g_message ("  No specific monitors to set up");
	}
}

static void
add_recurse_monitors (const gchar *name)
{
	GList *monitors;
	GList *l;

	monitors = tracker_module_config_get_monitor_recurse_directories (name);
	
	for (l = monitors; l; l = l->next) {
		GFile       *file;
		const gchar *path;

		path = l->data;

		g_message ("  Adding recurse directory monitor:'%s' (FIXME: Not finished)", path);

		file = g_file_new_for_path (path);
		tracker_monitor_add (file);
		g_object_unref (file);
	}

	g_list_free (monitors);

	if (!monitors) {
		g_message ("  No recurse monitors to set up");
	}
}

static void
process_module (TrackerProcessor *processor,
		const gchar      *module_name)
{
	TrackerProcessorPrivate *priv;

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

	g_message ("Processing module:'%s'", module_name);

	/* Set up monitors */

	/* Set up recursive monitors */

	/* Gets all files and directories */
	if (!tracker_crawler_start (priv->crawler, module_name)) {
		/* If there is nothing to crawl, we are done, process
		 * the next module.
		 */
		process_next_module (processor);
	}
}

static void
process_next_module (TrackerProcessor *processor)
{
	TrackerProcessorPrivate *priv;

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

	if (!priv->current_module) {
		priv->current_module = priv->modules;
	} else {
		priv->current_module = priv->current_module->next;
	}

	if (!priv->current_module) {
		priv->finished = TRUE;
		tracker_processor_stop (processor);
		return;
	}

	process_module (processor, priv->current_module->data);
}

#if 0

static void
file_queue_handler_set_up (TrackerCrawler *crawler)
{
	if (crawler->priv->files_queue_handle_id != 0) {
		return;
	}

	crawler->priv->files_queue_handle_id =
		g_timeout_add (FILES_QUEUE_PROCESS_INTERVAL,
			       file_queue_handler_cb,
			       crawler);
}

static void
indexer_check_files_cb (DBusGProxy *proxy,
			GError     *error,
			gpointer    user_data)
{
	GStrv files;

	files = (GStrv) user_data;

	if (error) {
		g_critical ("Could not send files to indexer to check, %s",
			    error->message);
		g_error_free (error);
	} else {
		g_debug ("Sent!");
	}
}

static void
indexer_get_running_cb (DBusGProxy *proxy,
			gboolean    running,
			GError     *error,
			gpointer    user_data)
{
	TrackerCrawler *crawler;
	GStrv           files;

	crawler = TRACKER_CRAWLER (user_data);

	if (error || !running) {
		g_message ("%s",
			   error ? error->message : "Indexer exists but is not available yet, waiting...");

		g_object_unref (crawler);
		g_clear_error (&error);

		return;
	}

	g_debug ("File check queue being processed...");
	files = tracker_dbus_async_queue_to_strv (crawler->priv->files,
						  FILES_QUEUE_PROCESS_MAX);

	g_debug ("File check queue processed, sending first %d to the indexer",
		 g_strv_length (files));

	org_freedesktop_Tracker_Indexer_files_check_async (proxy,
							   "files",
							   (const gchar **) files,
							   indexer_check_files_cb,
							   files);

	g_object_unref (crawler);
}

static gboolean
file_queue_handler_cb (gpointer user_data)
{
	TrackerCrawler *crawler;
	DBusGProxy     *proxy;
	gint            length;

	crawler = user_data;

	length = g_async_queue_length (crawler->priv->files);
	if (length < 1) {
		g_debug ("File check queue is empty... nothing to do");
		crawler->priv->files_queue_handle_id = 0;
		return FALSE;
	}

	/* Check we can actually talk to the indexer */
	proxy = tracker_dbus_indexer_get_proxy ();

	org_freedesktop_Tracker_Indexer_get_running_async (proxy,
							   indexer_get_running_cb,
							   g_object_ref (crawler));

	return TRUE;
}

#endif

static void
crawler_finished_cb (TrackerCrawler *crawler,
		     gpointer        user_data)
{
	TrackerProcessor *processor;
	
	processor = TRACKER_PROCESSOR (user_data);

	process_next_module (processor);
}

TrackerProcessor *
tracker_processor_new (TrackerConfig *config)
{
	TrackerProcessor        *processor;
	TrackerProcessorPrivate *priv;

	g_return_val_if_fail (TRACKER_IS_CONFIG (config), NULL);

	processor = g_object_new (TRACKER_TYPE_PROCESSOR, NULL);

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

	priv->config = g_object_ref (config);
	priv->crawler = tracker_crawler_new (config);

#ifdef HAVE_HAL
 	priv->hal = tracker_hal_new ();
	tracker_crawler_set_hal (priv->crawler, priv->hal);
#endif /* HAVE_HAL */

	g_signal_connect (priv->crawler, "finished",
			  G_CALLBACK (crawler_finished_cb),
			  processor);

	return processor;
}

void
tracker_processor_start (TrackerProcessor *processor)
{
	TrackerProcessorPrivate *priv;

	g_return_if_fail (TRACKER_IS_PROCESSOR (processor));

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

        g_message ("Starting to process %d modules...",
		   g_list_length (priv->modules));

	if (priv->timer) {
		g_timer_destroy (priv->timer);
	}

	priv->timer = g_timer_new ();

	priv->finished = FALSE;

	process_next_module (processor);
}

void
tracker_processor_stop (TrackerProcessor *processor)
{
	TrackerProcessorPrivate *priv;

	g_return_if_fail (TRACKER_IS_PROCESSOR (processor));

	priv = TRACKER_PROCESSOR_GET_PRIVATE (processor);

	if (!priv->finished) {
		tracker_crawler_stop (priv->crawler);
	}

	g_timer_stop (priv->timer);
	
	g_message ("Process %s %4.4f seconds",
		   priv->finished ? "finished in" : "stopped after",
		   g_timer_elapsed (priv->timer, NULL));
	
	g_signal_emit (processor, signals[FINISHED], 0);
}
