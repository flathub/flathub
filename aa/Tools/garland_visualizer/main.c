#include <stdio.h>
#include <assert.h>
#include <unistd.h>
#include <SDL/SDL.h>
#include <GL/gl.h>

#include "qcommon.h"

static const int win_w = 640, win_h = 480, hm_texnum = 1;

void gfx_init (void)
{
    assert (!SDL_Init (SDL_INIT_VIDEO));
    atexit (SDL_Quit);
    
    SDL_GL_SetAttribute (SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute (SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute (SDL_GL_DOUBLEBUFFER, 1);
    
    assert (SDL_SetVideoMode (win_w, win_h, 24, SDL_OPENGL | SDL_RESIZABLE));
    
    glClearColor (0, 1, 0, 1);
    glViewport (1, 1, win_w - 2, win_h - 2);
}

void gfx_set_background (char *path)
{
    int w, h;
    unsigned char *texdata;
    
    LoadTGA (path, &texdata, &w, &h);
    glBindTexture (GL_TEXTURE_2D, hm_texnum);
    glTexImage2D (GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, texdata);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri (GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    
    free (texdata);
}

static float bg_varray[4][4] = 
{
    {-1, -1,    0, 1},
    {1, -1,     1, 1},
    {1, 1,      1, 0},
    {-1, 1,     0, 0}
};

void gfx_draw_bg (void)
{
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (-1, 1, -1, 1, -1, 1);
    
    glBindTexture (GL_TEXTURE_2D, hm_texnum);
    glEnable (GL_TEXTURE_2D);
    
    glEnableClientState (GL_VERTEX_ARRAY);
    glVertexPointer (2, GL_FLOAT, sizeof (bg_varray[0]), &bg_varray[0][0]);
    glEnableClientState (GL_TEXTURE_COORD_ARRAY);
    glTexCoordPointer (2, GL_FLOAT, sizeof (bg_varray[0]), &bg_varray[0][2]);
    
    glDrawArrays (GL_QUADS, 0, 4);
    
    glDisableClientState (GL_TEXTURE_COORD_ARRAY);
    glDisableClientState (GL_VERTEX_ARRAY);
}

void gfx_draw_mesh (terraindata_t *mesh)
{
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    glOrtho (mesh->mins[0], mesh->maxs[0], mesh->mins[1], mesh->maxs[1], -1, 1);
    
    glDisable (GL_TEXTURE_2D);
    glLineWidth (1.0);
    glColor3f (1.0, 1.0, 1.0);
    
    glEnableClientState (GL_VERTEX_ARRAY);
    glVertexPointer (2, GL_FLOAT, 3 * sizeof (float), mesh->vert_positions);
    glPolygonMode (GL_BACK, GL_LINE);
    glDrawElements (GL_TRIANGLES, mesh->num_triangles*3, GL_UNSIGNED_INT, mesh->tri_indices);
    glPolygonMode (GL_BACK, GL_FILL);
    glDisableClientState (GL_VERTEX_ARRAY);
}

static int do_reduce = 0;
static int reduce_some = 0;

static void gfx_process_events (void)
{
    SDL_Event event;

    while (SDL_PollEvent (&event))
    {
        switch (event.type)
        {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym)
                {
                    case SDLK_SPACE:
                        do_reduce = !do_reduce;
                        break;
                    case SDLK_1: case SDLK_2: case SDLK_3: case SDLK_4:
                    case SDLK_5: case SDLK_6: case SDLK_7: case SDLK_8:
                    case SDLK_9:
                        reduce_some = pow (10, event.key.keysym.sym - SDLK_1);
                        break;
                    default:
                        break;
                }
                break;
            case SDL_VIDEORESIZE:
                if (event.resize.w < 3 || event.resize.h < 3)
                    break;
                assert (SDL_SetVideoMode (event.resize.w, event.resize.h, 24, SDL_OPENGL | SDL_RESIZABLE));
                glViewport (1, 1, event.resize.w - 2, event.resize.h - 2);
                break;
            case SDL_QUIT:
                exit (0);
        }
    }
}

static int orig_polycount;

void gfx_frame (terraindata_t *mesh)
{
    char caption[256];
    
    glDrawBuffer (GL_BACK);
    
    glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    glMatrixMode (GL_MODELVIEW);
    glLoadIdentity ();
    
    gfx_draw_bg ();
    gfx_draw_mesh (mesh);
    
    SDL_GL_SwapBuffers ();
    gfx_process_events ();
    
    snprintf (caption, sizeof (caption), "current: %d goal: %d start: %d", mesh->num_triangles, orig_polycount / 32, orig_polycount);
    SDL_WM_SetCaption (caption, NULL);
}

void visualizer_step (terraindata_t *out, qboolean export);

int main (int argc, char *argv[])
{
    char *buf;
    char *name = argv[1];
    int stopped_at_goal = 0;
    
    terraindata_t loaded;
    
    standin_init ();
    assert (argc == 2);
    
    gfx_init ();
    
    FS_LoadFile (name, (void**)&buf);
    
    LoadTerrainFile (&loaded, name, false, 2.0, 32, buf);
    gfx_set_background (loaded.hmtex_path);
    orig_polycount = loaded.num_triangles;
    
    for (;;)
    {
        if (do_reduce || reduce_some)
        {
            while (reduce_some > 1)
            {
                visualizer_step (&loaded, false);
                reduce_some--;
            }
            visualizer_step (&loaded, true);
            reduce_some = 0;
            if (loaded.num_triangles <= orig_polycount / 32 && !stopped_at_goal)
            {
                do_reduce = 0;
                stopped_at_goal = 1;
            }
        }
        gfx_frame (&loaded);
        
        usleep ((1000*1000)/120);
    }
    
    return 0;
}
