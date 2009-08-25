/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/*
 * Copyright (C) 2006, Mr Jamie McCracken (jamiemcc@gnome.org)
 * Copyright (C) 2008, Nokia

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

#ifndef DBUS_API_SUBJECT_TO_CHANGE
#define DBUS_API_SUBJECT_TO_CHANGE
#endif

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <locale.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#if defined(__linux__)
#include <linux/sched.h>
#endif
#include <sched.h>

#include <glib.h>
#include <glib/gi18n.h>
#include <glib/gstdio.h>

#include <libtracker-common/tracker-file-utils.h>
#include <libtracker-common/tracker-power.h>
#include <libtracker-common/tracker-storage.h>
#include <libtracker-common/tracker-ioprio.h>
#include <libtracker-common/tracker-log.h>
#include <libtracker-common/tracker-module-config.h>
#include <libtracker-common/tracker-ontology.h>

#include <libtracker-db/tracker-db-manager.h>
#include <libtracker-db/tracker-db-dbus.h>

#include <libtracker-data/tracker-data-manager.h>
#include <libtracker-data/tracker-data-backup.h>
#include <libtracker-data/tracker-data-query.h>

#include "tracker-dbus.h"
#include "tracker-config.h"
#include "tracker-events.h"
#include "tracker-main.h"
#include "tracker-push.h"
#include "tracker-backup.h"
#include "tracker-store.h"
#include "tracker-statistics.h"

#ifdef G_OS_WIN32
#include <windows.h>
#include <pthread.h>
#include "mingw-compat.h"
#endif

#define ABOUT								  \
	"Tracker " PACKAGE_VERSION "\n"

#define LICENSE								  \
	"This program is free software and comes without any warranty.\n" \
	"It is licensed under version 2 or later of the General Public "  \
	"License which can be viewed at:\n"				  \
	"\n"								  \
	"  http://www.gnu.org/licenses/gpl.txt\n"

#ifdef HAVE_HAL

typedef struct {
	gchar *udi;
	gchar *mount_point;
	gboolean no_crawling;
	gboolean was_added;
} MountPointUpdate;

#endif /* HAVE_HAL */

typedef struct {
	GMainLoop	 *main_loop;
	gchar		 *log_filename;

	gchar		 *data_dir;
	gchar		 *user_data_dir;
	gchar		 *sys_tmp_dir;
	gchar            *ttl_backup_file;
	
	gboolean          first_time_index;
	gboolean	  reindex_on_shutdown;
	gboolean          shutdown;
} TrackerMainPrivate;

/* Private */
static GStaticPrivate	     private_key = G_STATIC_PRIVATE_INIT;

/* Private command line parameters */
static gboolean		     version;
static gint		     verbosity = -1;
static gboolean		     low_memory;

static gboolean		     force_reindex;
static gboolean		     readonly_mode;

static GOptionEntry	     entries[] = {
	/* Daemon options */
	{ "version", 'V', 0,
	  G_OPTION_ARG_NONE, &version,
	  N_("Displays version information"),
	  NULL },
	{ "verbosity", 'v', 0,
	  G_OPTION_ARG_INT, &verbosity,
	  N_("Logging, 0 = errors only, "
	     "1 = minimal, 2 = detailed and 3 = debug (default = 0)"),
	  NULL },
	{ "low-memory", 'm', 0,
	  G_OPTION_ARG_NONE, &low_memory,
	  N_("Minimizes the use of memory but may slow indexing down"),
	  NULL },

	/* Indexer options */
	{ "force-reindex", 'r', 0,
	  G_OPTION_ARG_NONE, &force_reindex,
	  N_("Force a re-index of all content"),
	  NULL },
	{ "readonly-mode", 'n', 0,
	  G_OPTION_ARG_NONE, &readonly_mode,
	  N_("Only allow read based actions on the database"), NULL },
	{ NULL }
};

static void
private_free (gpointer data)
{
	TrackerMainPrivate *private;

	private = data;

	g_free (private->sys_tmp_dir);
	g_free (private->user_data_dir);
	g_free (private->data_dir);

	g_free (private->ttl_backup_file);
	g_free (private->log_filename);

	g_main_loop_unref (private->main_loop);

	g_free (private);
}

#ifdef HAVE_HAL

#if 0
static MountPointUpdate *
mount_point_update_new (const gchar *udi,
			const gchar *mount_point,
			gboolean no_crawling,
			gboolean was_added)
{
	MountPointUpdate *mpu;

	mpu = g_slice_new0 (MountPointUpdate);

	mpu->udi = g_strdup (udi);
	mpu->mount_point = g_strdup (mount_point);

	mpu->no_crawling = no_crawling;
	mpu->was_added = was_added;

	return mpu;
}

static void
mount_point_update_free (MountPointUpdate *mpu)
{
	if (!mpu) {
		return;
	}

	g_free (mpu->mount_point);
	g_free (mpu->udi);

	g_slice_free (MountPointUpdate, mpu);
}

static void
mount_point_set (MountPointUpdate *mpu)
{
	g_message ("Indexer has now set the state for the volume with UDI:");
	g_message (" %s", mpu->udi);
}

static void
mount_point_set_cb (DBusGProxy *proxy,
		    GError *error,
		    gpointer user_data)
{
	MountPointUpdate *mpu;

	mpu = user_data;

	if (error) {
		g_critical ("Indexer couldn't set volume state for:'%s' in database, %s",
			    mpu->udi,
			    error ? error->message : "no error given");

		g_error_free (error);

		tracker_shutdown ();
	} else {
		mount_point_set (mpu);
	}

	mount_point_update_free (mpu);
}
#endif

static void
mount_point_added_cb (TrackerStorage *hal,
		      const gchar    *udi,
		      const gchar    *mount_point,
		      gpointer	      user_data)
{
	/* TODO: Fix volume handling in tracker-store / tracker-indexer

	TrackerMainPrivate *private;
	MountPointUpdate *mpu;

	private = g_static_private_get (&private_key);

	g_message ("Indexer is being notified about added UDI:");
	g_message ("  %s", udi);

	mpu = mount_point_update_new (udi, mount_point, FALSE, TRUE);
	org_freedesktop_Tracker_Indexer_volume_update_state_async (tracker_dbus_indexer_get_proxy (), 
								   udi,
								   mount_point,
								   TRUE,
								   mount_point_set_cb,
								   mpu);
	 */
}

#if 0
static void
mount_point_set_and_signal_cb (DBusGProxy *proxy, 
			       GError     *error, 
			       gpointer    user_data)
{
	if (error) {
		g_critical ("Couldn't set mount point state, %s", 
			    error->message);
		g_error_free (error);
		g_free (user_data);
		return;
	}

	g_message ("Indexer now knows about UDI state:");
	g_message ("  %s", (gchar*) user_data);


	/* This is a special case, because we don't get the
	 * "Finished" signal from the indexer when we set something
	 * in the volumes table, we have to signal all clients from
	 * here that the statistics may have changed.
	 */
	tracker_statistics_signal ();

	g_free (user_data);
}
#endif

static void
mount_point_removed_cb (TrackerStorage  *hal,
			const gchar     *udi,
			const gchar     *mount_point,
			gpointer         user_data)
{
	/* TODO: Fix volume handling in tracker-store / tracker-indexer

	TrackerMainPrivate *private;
	MountPointUpdate *mpu;

	private = g_static_private_get (&private_key);

	g_message ("Indexer is being notified about removed UDI:");
	g_message ("  %s", udi);

	mpu = mount_point_update_new (udi, mount_point, FALSE, FALSE);
	org_freedesktop_Tracker_Indexer_volume_update_state_async (tracker_dbus_indexer_get_proxy (), 
								   udi,
								   mount_point,
								   FALSE,
								   mount_point_set_and_signal_cb,
								   mpu);
	 */
}

#endif /* HAVE_HAL */

static void
sanity_check_option_values (TrackerConfig *config)
{
	g_message ("General options:");
	g_message ("  Verbosity  ............................  %d",
		   tracker_config_get_verbosity (config));
	g_message ("  Low memory mode  ......................  %s",
		   tracker_config_get_low_memory_mode (config) ? "yes" : "no");

	g_message ("Store options:");
	g_message ("  Readonly mode  ........................  %s",
		   readonly_mode ? "yes" : "no");
}

static gboolean
shutdown_timeout_cb (gpointer user_data)
{
	g_critical ("Could not exit in a timely fashion - terminating...");
	exit (EXIT_FAILURE);

	return FALSE;
}

static void
signal_handler (int signo)
{
	static gboolean in_loop = FALSE;

	/* Die if we get re-entrant signals handler calls */
	if (in_loop) {
		_exit (EXIT_FAILURE);
	}

	switch (signo) {
	case SIGTERM:
	case SIGINT:
		in_loop = TRUE;
		tracker_shutdown ();

	default:
		if (g_strsignal (signo)) {
			g_print ("\n");
			g_print ("Received signal:%d->'%s'",
				 signo,
				 g_strsignal (signo));
		}
		break;
	}
}

static void
initialize_signal_handler (void)
{
#ifndef G_OS_WIN32
	struct sigaction act;
	sigset_t	 empty_mask;

	sigemptyset (&empty_mask);
	act.sa_handler = signal_handler;
	act.sa_mask    = empty_mask;
	act.sa_flags   = 0;

	sigaction (SIGTERM, &act, NULL);
	sigaction (SIGINT,  &act, NULL);
	sigaction (SIGHUP,  &act, NULL);
#endif /* G_OS_WIN32 */
}

static void
initialize_priority (void)
{
	/* Set disk IO priority and scheduling */
	tracker_ioprio_init ();

	/* NOTE: We only set the nice() value when crawling, for all
	 * other times we don't have a nice() value. Check the
	 * tracker-status code to see where this is done.
	 */
}

static void
initialize_locations (void)
{
	TrackerMainPrivate *private;
	gchar		   *filename;

	private = g_static_private_get (&private_key);

	/* Public locations */
	private->user_data_dir =
		g_build_filename (g_get_user_data_dir (),
				  "tracker",
				  "data",
				  NULL);

	private->data_dir =
		g_build_filename (g_get_user_cache_dir (),
				  "tracker",
				  NULL);

	filename = g_strdup_printf ("tracker-%s", g_get_user_name ());
	private->sys_tmp_dir =
		g_build_filename (g_get_tmp_dir (),
				  filename,
				  NULL);
	g_free (filename);

	private->ttl_backup_file = 
		g_build_filename (private->user_data_dir, 
				  "tracker-userdata-backup.ttl",
				  NULL);
}

static void
initialize_directories (void)
{
	TrackerMainPrivate *private;
	gchar		   *filename;

	private = g_static_private_get (&private_key);

	/* NOTE: We don't create the database directories here, the
	 * tracker-db-manager does that for us.
	 */

	g_message ("Checking directory exists:'%s'", private->user_data_dir);
	g_mkdir_with_parents (private->user_data_dir, 00755);

	g_message ("Checking directory exists:'%s'", private->data_dir);
	g_mkdir_with_parents (private->data_dir, 00755);

	/* Remove old tracker dirs */
	filename = g_build_filename (g_get_home_dir (), ".Tracker", NULL);

	if (g_file_test (filename, G_FILE_TEST_EXISTS)) {
		tracker_path_remove (filename);
	}

	g_free (filename);

	/* Remove database if we are reindexing */
	filename = g_build_filename (private->sys_tmp_dir, "Attachments", NULL);
	g_mkdir_with_parents (filename, 00700);
	g_free (filename);
}

static void
shutdown_databases (void)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	/* TODO port backup support */
#if 0
	/* If we are reindexing, save the user metadata  */
	if (private->reindex_on_shutdown) {
		tracker_data_backup_save (private->ttl_backup_file, NULL);
	}
#endif
}

static void
shutdown_locations (void)
{
	/* Nothing to do here, this is done by the private free func */
}

static void
shutdown_directories (void)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	/* If we are reindexing, just remove the databases */
	if (private->reindex_on_shutdown) {
		tracker_db_manager_remove_all (FALSE);
	}
}

#if 0

static const gchar *
get_ttl_backup_filename (void) 
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	return private->ttl_backup_file;
}

static void
backup_user_metadata (TrackerConfig   *config, 
		      TrackerLanguage *language)
{
	gboolean is_first_time_index;

	g_message ("Saving metadata in %s", get_ttl_backup_filename ());
	
	/*
	 *  Init the DB stack to get the user metadata
	 */

	tracker_data_manager_init (0, NULL, &is_first_time_index);
	
	/*
	 * If some database is missing or the dbs dont exists, we dont need
	 * to backup anything.
	 */
	if (is_first_time_index) {
		tracker_data_manager_shutdown ();
		return;
	}

	/* Actual save of the metadata */
	tracker_data_backup_save (get_ttl_backup_filename (), NULL);
	
	/* Shutdown the DB stack */
	tracker_data_manager_shutdown ();
}
#endif

#ifdef HAVE_HAL

static void
set_up_mount_points (TrackerStorage *hal)
{
	/* TODO: Fix volume handling in tracker-store / tracker-indexer

	TrackerMainPrivate *private;
	GError *error = NULL;
	GList *roots, *l;

	private = g_static_private_get (&private_key);

	 * Merging: This has something to do with "mount_points_to_set", which apparently
	 * we  don't have here in master apparently (but which tracker-0.6 uses)
	 *
	 * tracker_status_set_is_paused_for_dbus (TRUE);

	g_message ("Indexer is being notified to disable all volumes");
	org_freedesktop_Tracker_Indexer_volume_disable_all (tracker_dbus_indexer_get_proxy (), &error);

	if (error) {
		g_critical ("Indexer couldn't disable all volumes, %s",
			    error->message);
		g_error_free (error);
		tracker_shutdown ();
		return;
	}

	g_message ("   Done");

	roots = tracker_storage_get_removable_device_udis (hal);
	
	l = roots;

	while (l && !private->shutdown) {
		MountPointUpdate *mpu;

		mpu = mount_point_update_new (l->data,
					      tracker_storage_udi_get_mount_point (hal, l->data),
					      TRUE,
					      tracker_storage_udi_get_is_mounted (hal, l->data));

		g_message (" %s", mpu->udi);

		org_freedesktop_Tracker_Indexer_volume_update_state (tracker_dbus_indexer_get_proxy (),
									   mpu->udi,
									   mpu->mount_point,
									   mpu->was_added,
									   &error);
		if (error) {
			g_critical ("Indexer couldn't set volume state for:'%s' in database, %s",
				    mpu->udi,
				    error->message);

			g_error_free (error);

			tracker_shutdown ();
			break;
		} else {
			mount_point_set (mpu);
		}

		mount_point_update_free (mpu);
		l = l->next;
	}

	g_list_free (roots);
	 */

	/* Merging: tracker-0.6 appears to have code here that we don't have
	 *
	 * About "mount_points_up" */
}


#endif /* HAVE_HAL */

static GStrv
tracker_daemon_get_notifiable_classes (void)
{
	TrackerDBResultSet *result_set;
	GStrv               classes_to_signal = NULL;

	result_set = tracker_data_query_sparql ("SELECT ?class WHERE { ?class tracker:notify true }", NULL);

	if (result_set) {
		guint count = 0;

		classes_to_signal = tracker_dbus_query_result_to_strv (result_set, 0, &count);
		g_object_unref (result_set);
	}

	return classes_to_signal;
}

gint
main (gint argc, gchar *argv[])
{
	GOptionContext		   *context = NULL;
	GError			   *error = NULL;
	TrackerMainPrivate	   *private;
	TrackerConfig		   *config;
	TrackerPower		   *hal_power;
	TrackerStorage		   *hal_storage;
	TrackerDBManagerFlags	    flags = 0;
	gboolean		    is_first_time_index, need_journal = FALSE;

	g_type_init ();

	if (!g_thread_supported ()) {
		g_thread_init (NULL);
	}

	private = g_new0 (TrackerMainPrivate, 1);
	g_static_private_set (&private_key,
			      private,
			      private_free);

	dbus_g_thread_init ();

	setlocale (LC_ALL, "");

	bindtextdomain (GETTEXT_PACKAGE, LOCALEDIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);

	/* Set timezone info */
	tzset ();
	
	/* Translators: this messagge will apper immediately after the
	 * usage string - Usage: COMMAND <THIS_MESSAGE>
	 */
	context = g_option_context_new (_("- start the tracker daemon"));
	g_option_context_add_main_entries (context, entries, NULL);
	g_option_context_parse (context, &argc, &argv, &error);
	g_option_context_free (context);

	if (error) {
		g_printerr ("Invalid arguments, %s\n", error->message);
		g_clear_error (&error);
		return EXIT_FAILURE;
	}

	if (version) {
		/* Print information */
		g_print ("\n" ABOUT "\n" LICENSE "\n");
		return EXIT_SUCCESS;
	}

	g_print ("Initializing tracker-store...\n");

	initialize_signal_handler ();

	/* Check XDG spec locations XDG_DATA_HOME _MUST_ be writable. */
	if (!tracker_env_check_xdg_dirs ()) {
		return EXIT_FAILURE;
	}

	/* This makes sure we don't steal all the system's resources */
	initialize_priority ();

	/* This makes sure we have all the locations like the data
	 * dir, user data dir, etc all configured.
	 *
	 * The initialize_directories() function makes sure everything
	 * exists physically and/or is reset depending on various
	 * options (like if we reindex, we remove the data dir).
	 */
	initialize_locations ();

	/* Initialize major subsystems */
	config = tracker_config_new ();

	/* Daemon command line arguments */
	if (verbosity > -1) {
		tracker_config_set_verbosity (config, verbosity);
	}

	if (low_memory) {
		tracker_config_set_low_memory_mode (config, TRUE);
	}

	initialize_directories ();

	if (!tracker_dbus_init ()) {
		return EXIT_FAILURE;
	}

	/* Initialize other subsystems */
	tracker_log_init (tracker_config_get_verbosity (config),
			  &private->log_filename);
	g_print ("Starting log:\n  File:'%s'\n", private->log_filename);

	sanity_check_option_values (config);

#ifdef HAVE_HAL
	hal_power = tracker_power_new ();
	hal_storage = tracker_storage_new ();

	g_signal_connect (hal_storage, "mount-point-added",
			  G_CALLBACK (mount_point_added_cb),
			  NULL);
	g_signal_connect (hal_storage, "mount-point-removed",
			  G_CALLBACK (mount_point_removed_cb),
			  NULL);
#endif /* HAVE_HAL */

	flags |= TRACKER_DB_MANAGER_REMOVE_CACHE;

	if (force_reindex) {
		/* TODO port backup support
		backup_user_metadata (config, language); */

		flags |= TRACKER_DB_MANAGER_FORCE_REINDEX;
	}

	if (tracker_config_get_low_memory_mode (config)) {
		flags |= TRACKER_DB_MANAGER_LOW_MEMORY_MODE;
	}

	if (!tracker_data_manager_init (flags, NULL, &is_first_time_index, 
	                                &need_journal)) {
		return EXIT_FAILURE;
	}

	tracker_store_init (need_journal);

#ifdef HAVE_HAL
	/* We set up the mount points here. For the mount points, this
	 * means contacting the Indexer. This means that we have to
	 * have already initialised the databases if we are going to
	 * do that.
	 */
	set_up_mount_points (hal_storage);
#endif /* HAVE_HAL */

	if (private->shutdown) {
		goto shutdown;
	}

	/* Make Tracker available for introspection */
	if (!tracker_dbus_register_objects ()) {
		return EXIT_FAILURE;
	}

	tracker_events_init (tracker_daemon_get_notifiable_classes);
	tracker_push_init ();

	g_message ("Waiting for DBus requests...");

	/* Set our status as running, if this is FALSE, threads stop
	 * doing what they do and shutdown.
	 */
	if (!private->shutdown) {
		private->main_loop = g_main_loop_new (NULL, FALSE);
		g_main_loop_run (private->main_loop);
	}

shutdown:
	/*
	 * Shutdown the daemon
	 */
	g_message ("Shutdown started");

	tracker_store_shutdown ();

	g_timeout_add_full (G_PRIORITY_LOW, 5000, shutdown_timeout_cb, NULL, NULL);


	g_message ("Cleaning up");

	shutdown_databases ();
	shutdown_directories ();

	/* Shutdown major subsystems */

	tracker_push_shutdown ();
	tracker_events_shutdown ();

	tracker_dbus_shutdown ();
	tracker_data_manager_shutdown ();
	tracker_log_shutdown ();

#ifdef HAVE_HAL
	g_signal_handlers_disconnect_by_func (hal_storage,
					      mount_point_added_cb,
					      NULL);
	g_signal_handlers_disconnect_by_func (hal_storage,
					      mount_point_removed_cb,
					      NULL);

	g_object_unref (hal_power);
	g_object_unref (hal_storage);
#endif /* HAVE_HAL */

	g_object_unref (config);

	shutdown_locations ();

	g_print ("\nOK\n\n");

	return EXIT_SUCCESS;
}

void
tracker_shutdown (void)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	if (private) {
		if (private->main_loop) {
			g_main_loop_quit (private->main_loop);
		}

		private->shutdown = TRUE;
	}
}

const gchar *
tracker_get_data_dir (void)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	return private->data_dir;
}

const gchar *
tracker_get_sys_tmp_dir (void)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	return private->sys_tmp_dir;
}

void
tracker_set_reindex_on_shutdown (gboolean value)
{
	TrackerMainPrivate *private;

	private = g_static_private_get (&private_key);

	private->reindex_on_shutdown = value;
}

