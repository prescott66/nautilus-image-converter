/*
 *  nautilus-image-resize-dialog.c
 * 
 *  Copyright (C) 2004-2005 Jürg Billeter
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 *  Author: Jürg Billeter <j@bitron.ch>
 * 
 */

#ifdef HAVE_CONFIG_H
 #include <config.h> /* for GETTEXT_PACKAGE */
#endif

#include "nautilus-image-resizer.h"

#include <string.h>

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include <glade/glade.h>

#include <libgnomevfs/gnome-vfs.h>

#include <libnautilus-extension/nautilus-file-info.h>
 
#define PKGDATADIR DATADIR "/" PACKAGE

typedef struct _NautilusImageResizerPrivate NautilusImageResizerPrivate;

struct _NautilusImageResizerPrivate {
	GList *files;
	
	gchar *suffix;
	
	int images_resized;
	int images_total;
	gboolean cancelled;
	
	gchar *size;

	GtkDialog *resize_dialog;
	GtkRadioButton *default_size_radiobutton;
	GtkComboBox *size_combobox;
	GtkRadioButton *custom_size_radiobutton;
	GtkSpinButton *width_spinbutton;
	GtkSpinButton *height_spinbutton;
	GtkRadioButton *append_radiobutton;
	GtkEntry *name_entry;
	GtkRadioButton *inplace_radiobutton;

	GtkWidget *progress_dialog;
	GtkWidget *progress_bar;
	GtkWidget *progress_label;
};

#define NAUTILUS_IMAGE_RESIZER_GET_PRIVATE(o) (G_TYPE_INSTANCE_GET_PRIVATE ((o), NAUTILUS_TYPE_IMAGE_RESIZER, NautilusImageResizerPrivate))

G_DEFINE_TYPE (NautilusImageResizer, nautilus_image_resizer, G_TYPE_OBJECT)

enum {
	PROP_FILES = 1,
};

typedef enum {
	/* Place Signal Types Here */
	SIGNAL_TYPE_EXAMPLE,
	LAST_SIGNAL
} NautilusImageResizerSignalType;

static void
nautilus_image_resizer_finalize(GObject *object)
{
	NautilusImageResizer *dialog = NAUTILUS_IMAGE_RESIZER (object);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (dialog);
	
	g_free (priv->suffix);
		
	G_OBJECT_CLASS(nautilus_image_resizer_parent_class)->finalize(object);
}

static void
nautilus_image_resizer_set_property (GObject      *object,
                        guint         property_id,
                        const GValue *value,
                        GParamSpec   *pspec)
{
	NautilusImageResizer *dialog = NAUTILUS_IMAGE_RESIZER (object);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (dialog);

	switch (property_id) {
	case PROP_FILES:
		priv->files = g_value_get_pointer (value);
		priv->images_total = g_list_length (priv->files);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
}

static void
nautilus_image_resizer_get_property (GObject      *object,
                        guint         property_id,
                        GValue       *value,
                        GParamSpec   *pspec)
{
	NautilusImageResizer *self = NAUTILUS_IMAGE_RESIZER (object);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (self);

	switch (property_id) {
	case PROP_FILES:
		g_value_set_pointer (value, priv->files);
		break;
	default:
		/* We don't have any other property... */
		G_OBJECT_WARN_INVALID_PROPERTY_ID(object,property_id,pspec);
		break;
	}
}

static void
nautilus_image_resizer_class_init(NautilusImageResizerClass *klass)
{
	g_type_class_add_private (klass, sizeof (NautilusImageResizerPrivate));

	GObjectClass *object_class = G_OBJECT_CLASS(klass);
	GParamSpec *files_param_spec;

	object_class->finalize = nautilus_image_resizer_finalize;
	object_class->set_property = nautilus_image_resizer_set_property;
	object_class->get_property = nautilus_image_resizer_get_property;

	files_param_spec = g_param_spec_pointer ("files",
	"Files",
	"Set selected files",
	G_PARAM_CONSTRUCT_ONLY | G_PARAM_READWRITE);

	g_object_class_install_property (object_class,
	PROP_FILES,
	files_param_spec);
}

static void run_op (NautilusImageResizer *resizer);

static char *
nautilus_image_resizer_transform_uri (NautilusImageResizer *resizer, char *text_uri)
{
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);

	GnomeVFSURI *uri;
	GnomeVFSURI *parent;
	GnomeVFSURI *new_uri;
	char *escaped_basename;
	char *basename;
	char *extension;
	char *new_basename;
	char *new_text_uri;
	
	uri = gnome_vfs_uri_new (text_uri);
	
	parent = gnome_vfs_uri_get_parent (uri);
	
	escaped_basename = gnome_vfs_uri_extract_short_path_name (uri);
	basename = gnome_vfs_unescape_string (escaped_basename, "/");
	g_free (escaped_basename);
	gnome_vfs_uri_unref (uri);
	extension = g_strdup (strrchr (basename, '.'));
	if (extension != NULL)
		basename[strlen (basename) - strlen (extension)] = '\0';
	new_basename = g_strdup_printf ("%s%s%s", basename,
		priv->suffix == NULL ? ".tmp" : priv->suffix,
		extension == NULL ? "" : extension);
	g_free (basename);
	g_free (extension);
	
	new_uri = gnome_vfs_uri_append_file_name (parent, new_basename);
	gnome_vfs_uri_unref (parent);
	g_free (new_basename);
	
	new_text_uri = gnome_vfs_uri_to_string (new_uri, GNOME_VFS_URI_HIDE_NONE);
	gnome_vfs_uri_unref (new_uri);
	
	return new_text_uri;
}

static void
op_finished (GPid pid, gint status, gpointer data)
{
	NautilusImageResizer *resizer = NAUTILUS_IMAGE_RESIZER (data);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);
	
	gboolean retry = TRUE;
	
	NautilusFileInfo *file = NAUTILUS_FILE_INFO (priv->files->data);
	
	if (status != 0) {
		/* resizing failed */
		char *name = nautilus_file_info_get_name (file);

		GtkWidget *msg_dialog = gtk_message_dialog_new (GTK_WINDOW (priv->progress_dialog),
			GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
			GTK_BUTTONS_NONE,
			"'%s' cannot be resized. Check whether you have permission to write to this folder.",
			name);
		g_free (name);
		
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), _("_Skip"), 1);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		gtk_dialog_add_button (GTK_DIALOG (msg_dialog), _("_Retry"), 0);
		gtk_dialog_set_default_response (GTK_DIALOG (msg_dialog), 0);
		
		int response_id = gtk_dialog_run (GTK_DIALOG (msg_dialog));
		gtk_widget_destroy (msg_dialog);
		if (response_id == 0) {
			retry = TRUE;
		} else if (response_id == GTK_RESPONSE_CANCEL) {
			priv->cancelled = TRUE;
		} else if (response_id == 1) {
			retry = FALSE;
		}
		
	} else if (priv->suffix == NULL) {
		/* resize image in place */
		char *uri = nautilus_file_info_get_uri (file);
		char *new_uri = nautilus_image_resizer_transform_uri (resizer, uri);
		gnome_vfs_move (new_uri, uri, TRUE);
	}

	if (status == 0 || !retry) {
		/* image has been successfully resized (or skipped) */
		priv->images_resized++;
		priv->files = priv->files->next;
	}
	
	if (!priv->cancelled && priv->files != NULL) {
		/* process next image */
		run_op (resizer);
	} else {
		/* cancel/terminate operation */
		gtk_widget_destroy (priv->progress_dialog);
	}
}

static void
nautilus_image_resizer_cancel_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	NautilusImageResizer *resizer = NAUTILUS_IMAGE_RESIZER (user_data);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);
	
	priv->cancelled = TRUE;
	gtk_dialog_set_response_sensitive (dialog, GTK_RESPONSE_CANCEL, FALSE);
}

static void
run_op (NautilusImageResizer *resizer)
{
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);
	
	g_return_if_fail (priv->files != NULL);
	
	if (priv->progress_dialog == NULL) {
		GtkWidget *vbox;
		GtkWidget *label;
		
		priv->progress_dialog = gtk_dialog_new ();
		gtk_window_set_title (GTK_WINDOW (priv->progress_dialog), "Resizing files");
		gtk_dialog_add_button (GTK_DIALOG (priv->progress_dialog), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
		gtk_dialog_set_has_separator (GTK_DIALOG (priv->progress_dialog), FALSE);
		g_signal_connect (priv->progress_dialog, "response", G_CALLBACK (nautilus_image_resizer_cancel_cb), resizer);
		
		vbox = GTK_DIALOG (priv->progress_dialog)->vbox;
		gtk_container_set_border_width (GTK_CONTAINER (priv->progress_dialog), 5);
		gtk_box_set_spacing (GTK_BOX (vbox), 8);
		gtk_window_set_default_size (GTK_WINDOW (priv->progress_dialog), 400, -1);
		
		
		label = gtk_label_new ("<big><b>Resizing images</b></big>");
		gtk_label_set_use_markup (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0, 0);
		gtk_box_pack_start_defaults (GTK_BOX (vbox), label);

		priv->progress_bar = gtk_progress_bar_new ();
		gtk_box_pack_start (GTK_BOX (vbox), priv->progress_bar, FALSE, FALSE, 0);

		priv->progress_label = gtk_label_new ("");
		gtk_misc_set_alignment (GTK_MISC (priv->progress_label), 0, 0);
		gtk_box_pack_start_defaults (GTK_BOX (vbox), priv->progress_label);
		
		gtk_widget_show_all (priv->progress_dialog);
	}
	
	NautilusFileInfo *file = NAUTILUS_FILE_INFO (priv->files->data);

	char *uri = nautilus_file_info_get_uri (file);
	char *filename = gnome_vfs_get_local_path_from_uri (uri);
	char *new_uri = nautilus_image_resizer_transform_uri (resizer, uri);
	g_free (uri);
	char *new_filename = gnome_vfs_get_local_path_from_uri (new_uri);
	g_free (new_uri);
	
	/* FIXME: check whether new_uri already exists and provide "Replace _All", "_Skip", and "_Replace" options */
	
	gchar *argv[8];
	argv[0] = "/usr/bin/convert";
	argv[1] = "-size";
	argv[2] = priv->size;
	argv[3] = filename;
	argv[4] = "-resize";
	argv[5] = priv->size;
	argv[6] = new_filename;
	argv[7] = NULL;
	
	pid_t pid;

	if (!g_spawn_async (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL, NULL, &pid, NULL)) {
		// FIXME: error handling
		return;
	}
	
	g_free (filename);
	g_free (new_filename);
	
	g_child_watch_add (pid, op_finished, resizer);
	
	char *tmp;

	gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (priv->progress_bar), (double) (priv->images_resized + 1) / priv->images_total);
	tmp = g_strdup_printf (_("Resizing image: %d of %d"), priv->images_resized + 1, priv->images_total);
	gtk_progress_bar_set_text (GTK_PROGRESS_BAR (priv->progress_bar), tmp);
	g_free (tmp);
	
	char *name = nautilus_file_info_get_name (file);
	tmp = g_strdup_printf (_("<i>Resizing \"%s\"</i>"), name);
	g_free (name);
	gtk_label_set_markup (GTK_LABEL (priv->progress_label), tmp);
	g_free (tmp);
	
}

static void
nautilus_image_resizer_response_cb (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	NautilusImageResizer *resizer = NAUTILUS_IMAGE_RESIZER (user_data);
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);

	if (response_id == GTK_RESPONSE_OK) {
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->append_radiobutton))) {
			if (strlen (gtk_entry_get_text (priv->name_entry)) == 0) {
				GtkWidget *msg_dialog = gtk_message_dialog_new (GTK_WINDOW (dialog),
					GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR,
					GTK_BUTTONS_OK, _("Please enter a valid filename suffix!"));
				gtk_dialog_run (GTK_DIALOG (msg_dialog));
				gtk_widget_destroy (msg_dialog);
				return;
			}
			priv->suffix = g_strdup (gtk_entry_get_text (priv->name_entry));
		}
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (priv->default_size_radiobutton))) {
			priv->size = gtk_combo_box_get_active_text (GTK_COMBO_BOX (priv->size_combobox));
		} else {
			priv->size = g_strdup_printf ("%dx%d", (int) gtk_spin_button_get_value (priv->width_spinbutton), (int) gtk_spin_button_get_value (priv->height_spinbutton));
		}
		
		run_op (resizer);
	}

	gtk_widget_destroy (GTK_WIDGET (dialog));
}

static void
nautilus_image_resizer_init(NautilusImageResizer *resizer)
{
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);

	GladeXML *xml_dialog;

	xml_dialog = glade_xml_new (PKGDATADIR "/nautilus-image-resize.glade",
					  NULL, NULL);
	priv->resize_dialog = GTK_DIALOG (glade_xml_get_widget (xml_dialog, "resize_dialog"));
	priv->default_size_radiobutton = GTK_RADIO_BUTTON (glade_xml_get_widget (xml_dialog, "default_size_radiobutton"));
	priv->size_combobox = GTK_COMBO_BOX (glade_xml_get_widget (xml_dialog, "size_combobox"));
	gtk_combo_box_set_active  (priv->size_combobox, 4); /* 1024x768 */
	priv->custom_size_radiobutton = GTK_RADIO_BUTTON (glade_xml_get_widget (xml_dialog, "custom_size_radiobutton"));
	priv->width_spinbutton = GTK_SPIN_BUTTON (glade_xml_get_widget (xml_dialog, "width_spinbutton"));
	priv->height_spinbutton = GTK_SPIN_BUTTON (glade_xml_get_widget (xml_dialog, "height_spinbutton"));
	priv->append_radiobutton = GTK_RADIO_BUTTON (glade_xml_get_widget (xml_dialog, "append_radiobutton"));
	priv->name_entry = GTK_ENTRY (glade_xml_get_widget (xml_dialog, "name_entry"));
	priv->inplace_radiobutton = GTK_RADIO_BUTTON (glade_xml_get_widget (xml_dialog, "inplace_radiobutton"));
	
	g_signal_connect (G_OBJECT (priv->resize_dialog), "response", (GCallback) nautilus_image_resizer_response_cb, resizer);
}

NautilusImageResizer *
nautilus_image_resizer_new (GList *files)
{
	return g_object_new (NAUTILUS_TYPE_IMAGE_RESIZER, "files", files, NULL);
}

void
nautilus_image_resizer_show_dialog (NautilusImageResizer *resizer)
{
	NautilusImageResizerPrivate *priv = NAUTILUS_IMAGE_RESIZER_GET_PRIVATE (resizer);

	gtk_widget_show (GTK_WIDGET (priv->resize_dialog));
}
