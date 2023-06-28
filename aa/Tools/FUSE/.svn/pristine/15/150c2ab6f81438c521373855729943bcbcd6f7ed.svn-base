#include <glib.h>
#include <glib/gprintf.h>

#include "global.h"
#include "config.h"

#define FILEBUF 128 /* Chunk size of XML file to load and stream into parser */

/* Possibly the most basic XML load/save parser known to mankind ;) */

/* List of valid attributes */
typedef enum
{
    ATTRIB_NONE,
    ATTRIB_NAME,
    ATTRIB_WINDOWS,
    ATTRIB_LINUX,
    ATTRIB_MASTER,
    ATTRIB_GAME
}
tAttrib;

typedef struct _tUserData
{
    GData **config; /* Datalist of configured games */
    tGameConfig *currentconfig; /* Pointer to structure to be filled from current data between <GAME> tags */
    tAttrib attrib; /*  Attribute value associated with current XML element */
    guint mastercount; /* Keeps running total of master servers loaded from file */
    guint servercount; /* Keeps running total of game servers loaded from file */
} tUserData;

void CONFIG_Free(gpointer ptr);

/* Called on parse errors */
void FUSEXML_Error(GMarkupParseContext *context, GError *error, gpointer user_data)
{
    POPUP("XML Loader: Error\n\n%s", error->message);
}

void FUSEXML_Start(GMarkupParseContext *context,
                          const gchar         *element_name,
                          const gchar        **attribute_names,
                          const gchar        **attribute_values,
                          gpointer             user_data,
                          GError             **error)
{
    guint i;
    tUserData *ud = (tUserData *)user_data;

    ud->attrib = ATTRIB_NONE;


//    POPUP("FUSEXML_Start() - element_name=%s, last attrib = %u", element_name, ud->attrib);
    /* Check for valid attibutes */
    if  (g_ascii_strcasecmp(element_name, "GAME") == 0)
    {
        if(ud->currentconfig != NULL) /* Already within <GAME> tags */
        {
            POPUP_ERROR("XMLLoader: Error: <GAME> elements may not be nested");
            ud->currentconfig = NULL; /* Prevent any further configuration within this element */
            return;
        }
        for(i=0; attribute_names[i] != NULL; i++)
            if(g_ascii_strcasecmp(attribute_names[i], "NAME")==0)
            {
                tGameConfig *tmp;
                tmp = g_malloc(sizeof(tGameConfig));
                if(tmp != NULL)
                {
                    g_datalist_set_data_full(ud->config, g_strdup(attribute_values[i]), tmp, CONFIG_Free);
                    ud->currentconfig = tmp;
                    /* Clear all other config pointers (checked again on close tag) */
                    ud->currentconfig->protocol = NULL;
                    ud->currentconfig->launch = NULL;
                    ud->currentconfig->exewin = NULL;
                    ud->currentconfig->exelin = NULL;
                    ud->currentconfig->pathwin = NULL;
                    ud->currentconfig->pathlin = NULL;
                    ud->mastercount = 0;
                    ud->servercount = 0;
                    ud->attrib = ATTRIB_NONE;
                }
            }

        if(ud->currentconfig == NULL)
        {
            POPUP("XML Loader: Error (%s): You must specify a NAME attribute with this element\n\nEG: <GAME NAME=\"Alien Arena\">", element_name);
        }
        else
        {
            for(i=0; attribute_names[i] != NULL; i++)
                if(g_ascii_strcasecmp(attribute_names[i], "PROTOCOL")==0)
                    ud->currentconfig->protocol = g_strdup(attribute_values[i]);
        }
        if(ud->currentconfig == NULL)
            POPUP("XML Loader: Error (%s): You must specify a NAME attribute with this element\n\nEG: <GAME NAME=\"Alien Arena\">", element_name);

    }
    else if ( (g_ascii_strcasecmp(element_name, "EXECUTABLE") == 0) || (g_ascii_strcasecmp(element_name, "PATH") == 0))
    {
        for(i=0; attribute_names[i] != NULL; i++)
            if(g_ascii_strcasecmp(attribute_names[i], "PLATFORM")==0)
			{
                if(g_ascii_strcasecmp(attribute_values[i], "Windows") == 0)
                    ud->attrib = ATTRIB_WINDOWS;
                else if(g_ascii_strcasecmp(attribute_values[i], "Linux") == 0)
                    ud->attrib = ATTRIB_LINUX;
			}
        if(ud->attrib == ATTRIB_NONE)
            POPUP("XML Loader: Error (%s): You must specify attribute\n\n  PLATFORM=\"Windows\" or PLATFORM=\"Linux\"\n\nwith EXECUTABLE or PATH elements.\n\nEG: <EXECUTABLE PLATFORM=\"Windows\">crx.exe</EXECUTABLE>", element_name);
    }
    else if (g_ascii_strcasecmp(element_name, "SERVER") == 0)
    {
        for(i=0; attribute_names[i] != NULL; i++)
            if(g_ascii_strcasecmp(attribute_names[i], "TYPE")==0)
            {
			    if(g_ascii_strcasecmp(attribute_values[i], "Master") == 0)
                    ud->attrib = ATTRIB_MASTER;
                else if(g_ascii_strcasecmp(attribute_values[i], "Game") == 0)
                    ud->attrib = ATTRIB_GAME;
			}
        if(ud->attrib == ATTRIB_NONE)
            POPUP("XML Loader: Error (%s): You must specify attribute\n\n  TYPE=\"Master\" or TYPE=\"Game\"\n\nwith SERVER elements.\n\nEG: <SERVER TYPE=\"Master\">master.corservers.com:27900</SERVER>", element_name);
    }
}

/* Called for character data */
/* text is not /0 terminated */
void FUSEXML_Text(GMarkupParseContext *context,
                          const gchar         *text,
                          gsize                text_len,
                          gpointer             user_data,
                          GError             **error)
{
    tUserData *ud = (tUserData *)user_data;
    const gchar *element_name = g_markup_parse_context_get_element(context);
    if(ud->currentconfig == NULL)
        return;

    //POPUP("FUSEXML_Text() - text=%s, current element=%s", text, element == NULL ? "" : element_name);
    if (g_ascii_strcasecmp(element_name, "EXECUTABLE") == 0)
    {
        if(ud->attrib == ATTRIB_WINDOWS)
            ud->currentconfig->exewin = g_strndup(text, text_len);
        else if(ud->attrib == ATTRIB_LINUX)
            ud->currentconfig->exelin = g_strndup(text, text_len);
    }
    else if (g_ascii_strcasecmp(element_name, "LAUNCH") == 0)
    {
        ud->currentconfig->launch = g_strndup(text, text_len);
    }
    else if (g_ascii_strcasecmp(element_name, "PATH") == 0)
    {
        if(ud->attrib == ATTRIB_WINDOWS)
            ud->currentconfig->pathwin = g_strndup(text, text_len);
        else if(ud->attrib == ATTRIB_LINUX)
            ud->currentconfig->pathlin = g_strndup(text, text_len);
    }
    else if (g_ascii_strcasecmp(element_name, "SERVER") == 0)
    {
        if(ud->attrib == ATTRIB_MASTER)
        {
            if(ud->mastercount < MAXMASTERS)
                ud->currentconfig->masters[ud->mastercount++] = g_strndup(text, text_len);
            else
                POPUP("XML Loader: Only %u <SERVER TYPE=\"Master\"> entries allowed\n", MAXMASTERS);
        }
        else if(ud->attrib == ATTRIB_GAME)
        {
            if(ud->servercount < MAXGAMES)
                ud->currentconfig->gameservers[ud->servercount++] = g_strndup(text, text_len);
            else
                POPUP("XML Loader: Only %u <SERVER TYPE=\"Game\"> entries allowed\n", MAXGAMES);
        }
    }
}

void FUSEXML_End(GMarkupParseContext *context,
                          const gchar         *element_name,
                          gpointer             user_data,
                          GError             **error)
{
    tUserData *ud = (tUserData *)user_data;

    if (g_ascii_strcasecmp(element_name, "GAME") == 0)
    {
        /* Set sensible defaults for anything the XML file missed, and NULL terminate master/games lists */
        if(ud->currentconfig->protocol == NULL)
            ud->currentconfig->protocol = g_strdup_printf("q2");
        if(ud->currentconfig->launch == NULL)
            ud->currentconfig->launch = g_strdup_printf("+connect %%s");
        if(ud->currentconfig->exewin == NULL)
            ud->currentconfig->exewin = g_strdup_printf("UNDEFINED");
        if(ud->currentconfig->exelin == NULL)
            ud->currentconfig->exelin = g_strdup_printf("UNDEFINED");
        if(ud->currentconfig->pathwin == NULL)
            ud->currentconfig->pathwin = g_strdup_printf("UNDEFINED");
        if(ud->currentconfig->pathlin == NULL)
            ud->currentconfig->pathlin = g_strdup_printf("UNDEFINED");
        ud->currentconfig->masters[ud->mastercount] = NULL;
        ud->currentconfig->gameservers[ud->servercount] = NULL;
        #if 0
        POPUP("FUSEXML_End() Server config:\n\nprotocol=%s\nexewin=%s\nexelin=%s\npathwin=%s\npathlin=%s\nnummasters=%u\nnumgames=%u",
                ud->currentconfig->protocol == NULL ? "NULL" : ud->currentconfig->protocol,
                ud->currentconfig->exewin   == NULL ? "NULL" : ud->currentconfig->exewin,
                ud->currentconfig->exelin   == NULL ? "NULL" : ud->currentconfig->exelin,
                ud->currentconfig->pathwin  == NULL ? "NULL" : ud->currentconfig->pathwin,
                ud->currentconfig->pathlin  == NULL ? "NULL" : ud->currentconfig->pathlin,
                g_strv_length(ud->currentconfig->masters),
                g_strv_length(ud->currentconfig->gameservers) );
        #endif
        ud->currentconfig = NULL;
    }
}

gboolean Config_LoadGames(GData **gameconfigs, gchar *filename)
{
    GMarkupParser xmlparser;
    GMarkupParseContext* xmlcontext;
    GError *err = NULL;

    FILE *file;
    gchar *buffer[FILEBUF];
    guint len;

    tUserData ud;

    g_datalist_clear(gameconfigs);

    ud.config = gameconfigs;
    ud.currentconfig = NULL;
    ud.mastercount = 0;
    ud.servercount = 0;
    ud.attrib = ATTRIB_NONE;

    file = fopen(filename, "rb");
    if (file == NULL)
    {
        POPUP_ERROR("XML Loader: Unable to open file \"%s\"", filename);
        return FALSE;
    }

    xmlparser.start_element = FUSEXML_Start;        /* Called for open tags <foo bar="baz"> */
    xmlparser.end_element   = FUSEXML_End;                 /* Called for close tags </foo> */
    xmlparser.text          = FUSEXML_Text;         /* Called for character data (no /0 termination) */
    xmlparser.passthrough   = NULL;                 /* Called for comments etc */
    xmlparser.error         = FUSEXML_Error;        /* Called on error */

    xmlcontext = g_markup_parse_context_new(&xmlparser,
                                            0,      /* GMarkupParseFlags flags */
                                            &ud,   /* gpointer user_data */
                                            NULL);  /* GDestroyNotify user_data_dnotify */

    /* Pull in chunks of file and feed into the parser */
    while((len = fread(buffer, 1, FILEBUF, file)) > 0)
    {
        if(!g_markup_parse_context_parse(xmlcontext, (const gchar *)buffer, len, &err))
        {
            POPUP("XML Loader: g_markup_parse_context_parse() failed");
            break;
        }
    }

    if(!g_markup_parse_context_end_parse(xmlcontext, &err))
    {
        POPUP("XML Loader: Unclosed XML elements detected");
    }

    fclose(file);
    g_markup_parse_context_free (xmlcontext);

    return TRUE;
}

void CONFIG_Free(gpointer ptr)
{
    tGameConfig *config = (tGameConfig *)ptr;
    if(config == NULL)
        return;

    g_free(config->protocol);
    g_free(config->launch);
    g_free(config->exewin);
    g_free(config->exelin);
    g_free(config->pathwin);
    g_free(config->pathlin);
    g_strfreev(config->masters);
    g_strfreev(config->gameservers);
    g_free(config);
}

