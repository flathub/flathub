#include "project.h"
#include "retrospriteeditor-nes-list-palletes.h"
#include "retrospriteeditor-nes-item-pallete.h"
#include "retrospriteeditor-nes-palette.h"
#include <libxml/xmlwriter.h>
#include <libxml/xmlreader.h>
#include <stdio.h>
#include <string.h>

struct Project {
	char *folder_path;
	char *name;
	char *fullpath_to_project;
	char *file_to_export;
	int p0[4];
	int p1[4];
	int p2[4];
	int p3[4];
	int np0;
	int np1;
	int np2;
	int np3;
};

struct Project *prj;

void project_free (void)
{
	if (prj->folder_path)
		g_free (prj->folder_path);

	if (prj->name)
		g_free (prj->folder_path);
}

static void
project_alloc (void)
{
	prj = g_malloc0 (sizeof (struct Project));
}

static void
project_free_and_alloc (void)
{
	project_free ();
	project_alloc ();
}

static void
write_header (xmlTextWriterPtr *writ)
{
	xmlTextWriterPtr writer = *writ;

	writer = xmlNewTextWriterFilename (prj->fullpath_to_project, 0);
	xmlTextWriterStartDocument (writer, NULL, "UTF-8", NULL);
	xmlTextWriterStartElement (writer, "RetroSpriteEditor");
	xmlTextWriterStartElement (writer, "ProjectSettings");
			xmlTextWriterWriteElement (writer, "platform_id", "0");
			xmlTextWriterWriteElement (writer, "project_name", prj->name);
			xmlTextWriterWriteElement (writer, "project_folder", prj->folder_path);
			xmlTextWriterWriteElement (writer, "fullpath", prj->fullpath_to_project);
	xmlTextWriterEndElement (writer);
	xmlTextWriterStartElement (writer, "ExportSettings");
			xmlTextWriterWriteElement (writer, "outfile", prj->file_to_export);
	xmlTextWriterEndElement (writer);

	*writ = writer;
}

static void write_palettes (xmlTextWriterPtr writer);

void 
project_set_folder_and_name (const char *folder, const char *name)
{
	if (prj) {
		project_free_and_alloc ();
	} else {
		project_alloc ();
	}

	prj->folder_path = g_strdup (folder);
	prj->name = g_strdup (name);
	prj->fullpath_to_project = g_strdup_printf ("%s/%s.rse", folder, name);
	prj->file_to_export = g_strdup_printf ("%s/%s.bin", folder, "out_data");

	xmlTextWriterPtr writer;
	write_header (&writer);
	//write_palettes (writer);
	xmlTextWriterEndElement (writer);
	xmlTextWriterEndDocument (writer);
	xmlFreeTextWriter (writer);
}

char *
project_get_filepath_to_export ()
{
	if (prj->fullpath_to_project == NULL)
		return NULL;

	return prj->file_to_export;
}

static xmlChar *pname;

enum {
	TYPE_INT,
	TYPE_STRING,
	N_TYPE
};

static void
check_and_write (xmlChar *name, char *sname, xmlChar *value, void **v, int type)
{
	char *v0 = NULL;
	int  v1 = 0;
	if (strncmp (name, sname, strlen (sname) + 1))
		return;

	switch (type) {
		case TYPE_STRING:
			v0 = g_strdup (value);
			*v = v0;
			break;
		case TYPE_INT:
			v1 = atoi (value);
			*v = &v1;
			break;
	}
}

static void
handle_name_value (xmlChar *name, xmlChar *value)
{
	check_and_write (name, "project_name", value, (void **) &prj->name, TYPE_STRING);
	check_and_write (name, "project_folder", value, (void **) &prj->folder_path, TYPE_STRING);
	check_and_write (name, "fullpath", value, (void **) &prj->fullpath_to_project, TYPE_STRING);
	check_and_write (name, "outfile", value, (void **) &prj->file_to_export, TYPE_STRING);
	if (!strncmp (name, "p00", 4)) {
		prj->p0[prj->np0++] = atoi (value);
	}
	if (!strncmp (name, "p01", 4)) {
		prj->p1[prj->np1++] = atoi (value);
	}
	if (!strncmp (name, "p02", 4)) {
		prj->p2[prj->np2++] = atoi (value);
	}
	if (!strncmp (name, "p03", 4)) {
		prj->p3[prj->np3++] = atoi (value);
	}
}

static void
process_node (xmlTextReaderPtr reader)
{
	const xmlChar *name, *value;
	name = xmlTextReaderConstName (reader);
	value = xmlTextReaderConstValue (reader);

	if (value == NULL) {
		pname = name;
	} else if (!strncmp (name, "#text", 6)) {
		//g_print ("%s == %s\n", pname, value);
		handle_name_value (pname, value);
	}
}

static void
read_tilemap_and_set_nes (void)
{
	GFile *file = g_file_new_for_path (prj->file_to_export);
	GFileInputStream *input = g_file_read (file,
			NULL,
			NULL);

	unsigned char *data = g_malloc0 (8192);
	gsize r = g_input_stream_read (G_INPUT_STREAM (input),
			data,
			8192,
			NULL,
			NULL);

	g_input_stream_close (G_INPUT_STREAM (input), NULL, NULL);

	int blkx = 0;
	int blky = 0;
	int yy = 0;
	int xx = 0;
	unsigned char *t = data;
	int zx = 0;
	int mm = 0;
	for (int i = 0; i < 2; i++) {
		blkx = 0;
		blky = 0;
		NesTilePoint *p = nes_get_map (i);
		for (int tile = 0; tile < 256; tile++) {
			int index = 0;

			for (int y = 0; y < 8; y++) {
				for (int x = 7; x >= 0; x--) {
					int found = 0;
					unsigned char bit = 1 << x;
					if ((t[y] & bit) && (t[y + 8] & bit)) {
						index = 3;
						found = 1;
					} else {
						if ((t[y] & bit)) {
							index = 1;
							found = 1;
						}
						if ((t[y + 8] & bit)) {
							index = 2;
							found = 1;
						}
					}

					yy = y;
					xx = 7 - x;

					if (found > 0) {
					NesParamPoint n;
			    n.blockx = blkx;
    			n.blocky = blky;
    			n.x = xx;
    			n.y = yy;
    			RetrospriteeditorNesPalette *nes = get_nes ();
    			nes_set_color_with_map (nes, &n, index + 1, i);
					}
				}
			}
			t += 16;
			blkx++;
			if (blkx >= 16) {
				blkx = 0;
				blky++;
			}
		}
	}

	g_free (data);
}

void
project_open_nes (char *filepath)
{
	if (prj == NULL)
		prj = g_malloc0 (sizeof (struct Project));

	xmlTextReaderPtr reader;
	reader = xmlNewTextReaderFilename (filepath);
	int ret = xmlTextReaderRead (reader);
	while (ret == 1) {
		process_node (reader);
		ret = xmlTextReaderRead (reader);
	}

	xmlFreeTextReader (reader);

	GtkWidget **items = nes_list_pallete_get_items ();
	for (guint32 i = 0; i < 4; i++) {
		guint32 *color_index = item_nes_pallete_get_colour_index (RETROSPRITEEDITOR_NES_ITEM_PALLETE (items[i]));
		*(color_index + 0) = prj->p0[i];
		*(color_index + 1) = prj->p1[i];
		*(color_index + 2) = prj->p2[i];
		*(color_index + 3) = prj->p3[i];
		item_nes_pallete_get_color_from_index (RETROSPRITEEDITOR_NES_ITEM_PALLETE (items[i]));
	}

	read_tilemap_and_set_nes ();
}

static void
write_palettes (xmlTextWriterPtr writer)
{
	GtkWidget **items = nes_list_pallete_get_items ();
	xmlTextWriterStartElement (writer, "Palettes");
	for (guint32 i = 0; i < 4; i++) {
		guint32 *color_index = item_nes_pallete_get_colour_index (RETROSPRITEEDITOR_NES_ITEM_PALLETE (items[i]));
		char id[15];
		snprintf (id, 15, "Palette%02d", i);
		xmlTextWriterStartElement (writer, id);
		for (guint32 m = 0; m < 4; m++) {
			char index_num[15];
			char num[15];
			snprintf (index_num, 15, "%d", *(color_index + m));
			snprintf (num, 15, "p%02d", m);
			xmlTextWriterWriteElement (writer, num, index_num);
		}
		xmlTextWriterEndElement (writer);
	}
	xmlTextWriterEndElement (writer);
}

void
project_save_palettes ()
{
	xmlTextWriterPtr writer;
	write_header (&writer);
	write_palettes (writer);
	xmlTextWriterEndElement (writer);
	xmlTextWriterEndDocument (writer);
	xmlFreeTextWriter (writer);
}
