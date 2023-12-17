#include <SDL2/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>

#include <SDL_opengl.h>
#include <SDL_opengl_glext.h>

#include "libretro.h"

#include "defs.h"
#include "structs.h"

extern bool quit;

//fifo
void fifo_init(fifo_t * f, char * buf, int size);
int fifo_read(fifo_t * f, void * buf, int nbytes);
int fifo_write(fifo_t * f, const void * buf, int nbytes);

//video
void resize_cb(int w, int h);
void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch);
void video_deinit();
void video_configure(const struct retro_game_geometry *geom);
void video_setgeometry(int w,int h);
bool video_set_pixel_format(unsigned format);
void resize_to_aspect(double ratio, int sw, int sh, int *dw, int *dh);
void video_render();
void prepareScene(void);
void presentScene(void);

//input
int doInput(void);
void init_keyboards();

//audio
void audio_init(int frequency);
void audio_deinit();
bool audio_stop();
bool audio_start();
bool audio_alive();
size_t audio_write(const void *buf, unsigned frames);

//text
void initFonts(void);
void drawText(int x, int y, int r, int g, int b, int align,char *format, ...);
void drawTextScale(int x, int y, int r, int g, int b, int align,int scale,char *format, ...);
void drawChar(int x, int y, int r, int g, int b, char c);
void drawCharScale(int x, int y, int r, int g, int b, char c,int scale);
void deinitFonts(void);

//utli
void die(const char *fmt, ...);

// widget
void initWidgets(void);
void cleanWidgets();
Widget *createWidget(char *name);

//menu
void menu_setDelegate();
void initMenu(void);

//emu
void emulation_pause();
void emulation_start();
void emulation_cleanup(void);
void emulation_quit();

//core
void core_run();
void core_reset();
int core_main(int argc, char *argv[]);
void core_unload();
void save_savestate(const char *savestated);
void options_deinit();

//core-options
void split(char *string,const char sep[],int ind);
void freesuboptval(int ind);

//opengl
int initGL();
void drawTextureGL(GLuint backBuffer);
bool initGLExtensions();
void refresh_vertex_data() ;
void deinitGL();
void change_shaders();
void blitFontGL(GLuint backBuffer, int idx, int x, int y,float scale);
void RestorePrgGL();
void SetFontColorGL(int r,int  g,int  b);
