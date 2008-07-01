#include <glib.h>
#include <glib/gtestutils.h>

#include <libtracker-common/tracker-file-utils.h>
#include <tracker-test-helpers.h>
#include <string.h>

GSList *
array_as_list (gchar **array) 
{
        gint i;
        GSList *result = NULL;

        for (i = 0; array[i] != NULL; i++) {
                result = g_slist_prepend (result, g_strdup(array[i]));
                
        } 
        
        return result;
}

gboolean
string_in_list (GSList *list, const gchar *string) {

        GSList *it;
        for ( it = list; it != NULL; it = it->next) {
                if (strcmp (it->data, string) == 0) {
                        return TRUE;
                }
        }
        return FALSE;
}

static void
test_path_list_filter_duplicates ()
{
        gchar *input_roots [] = {"/home", "/home/ivan", "/tmp", "/usr/", "/usr/share/local", NULL};

        GSList *input_as_list = NULL;
        GSList *result;
        input_as_list = array_as_list (input_roots);


        result = tracker_path_list_filter_duplicates (input_as_list);
        // Expected /home /tmp /usr
        g_assert_cmpint (3, ==, g_slist_length (result));
        
        g_assert (string_in_list (result, "/home/"));
        g_assert (string_in_list (result, "/tmp/"));
        g_assert (string_in_list (result, "/usr/"));


        g_slist_foreach (input_as_list, g_free, NULL);
}

static void
test_path_evaluate_name ()
{
        gchar *result;
        
        const gchar *home = g_getenv ("HOME");
        const gchar *pwd = g_getenv ("PWD");

        const gchar *test = "/one/two";
        gchar *parent_dir;

        g_setenv ("TEST_TRACKER_DIR", test, TRUE);
        

        result = tracker_path_evaluate_name ("/home/user/all/ok");
        tracker_test_helpers_cmpstr_equal (result, "/home/user/all/ok");
        g_free (result);

        /* 
           The result of this test and the next one are not consistent! 
           Must it remove the end '/' or not?
         */
        result = tracker_path_evaluate_name ("/home/user/all/dir/");
        tracker_test_helpers_cmpstr_equal (result, "/home/user/all/dir");
        g_free (result);

        result = tracker_path_evaluate_name ("~/all/dir/");
        tracker_test_helpers_cmpstr_equal (result, 
                                           g_build_path (G_DIR_SEPARATOR_S, home, "/all/dir/", NULL));
        g_free (result);

        result = tracker_path_evaluate_name ("$HOME/all/dir/");
        tracker_test_helpers_cmpstr_equal (result, 
                                           g_build_path (G_DIR_SEPARATOR_S, home, "/all/dir", NULL));
        g_free (result);

        result = tracker_path_evaluate_name ("${HOME}/all/dir/");
        tracker_test_helpers_cmpstr_equal (result, 
                                           g_build_path (G_DIR_SEPARATOR_S, home, "/all/dir", NULL));
        g_free (result);

        result = tracker_path_evaluate_name ("./test/current/dir");
        tracker_test_helpers_cmpstr_equal (result,
                                           g_build_path (G_DIR_SEPARATOR_S, pwd, "/test/current/dir", NULL));
        g_free (result);

        result = tracker_path_evaluate_name ("$TEST_TRACKER_DIR/test/dir");
        tracker_test_helpers_cmpstr_equal (result,
                                           g_build_path (G_DIR_SEPARATOR_S, test, "/test/dir", NULL));
        g_free (result);


        result = tracker_path_evaluate_name ("../test/dir");
        parent_dir = g_path_get_dirname (pwd);
        tracker_test_helpers_cmpstr_equal (result,
                                           g_build_path (G_DIR_SEPARATOR_S, parent_dir, "/test/dir", NULL));
        g_free (result);
        g_free (parent_dir);


        result = tracker_path_evaluate_name ("");
        g_assert (!result);

        result = tracker_path_evaluate_name (NULL);
        g_assert (!result);

        result = tracker_path_evaluate_name (tracker_test_helpers_get_nonutf8 ());
        tracker_test_helpers_cmpstr_equal (result,
                                           tracker_test_helpers_get_nonutf8 ());
        g_unsetenv ("TEST_TRACKER_DIR");
}


int
main (int argc, char **argv) {

        int result;

	g_type_init ();
	g_test_init (&argc, &argv, NULL);


        g_test_add_func ("/tracker/libtracker-common/tracker-file-utils/path_evaluate_name",
                         test_path_evaluate_name);

        g_test_add_func ("/tracker/libtracker-common/tracker-file-utils/path_list_filter_duplicates",
                         test_path_list_filter_duplicates);

        result = g_test_run ();

        return result;
}
