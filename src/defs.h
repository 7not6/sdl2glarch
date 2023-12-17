#define STRNCPY(dest, src, n) strncpy(dest, src, n); dest[n - 1] = '\0'
#define FPS_INTERVAL 1.0
	
#define MAXK 136
#define MAX_KEYBOARD_KEYS 350
#define MAX_NAME_LENGTH        32
#define MAX_LINE_LENGTH        1024
#define MAX_OPTNAME_LENGTH	128
#define MAX_ENTRIES_PER_PAGE 20
#define MAX_SHADERS 4

#define GLYPH_HEIGHT 16
#define GLYPH_WIDTH  10
#define FONT_HEIGHT 256
#define FONT_WIDTH 160
#define FONT_BOXW (FONT_WIDTH/GLYPH_WIDTH)
#define FONT_BOXH (FONT_HEIGHT/GLYPH_HEIGHT)

#define FIFO_READ_AVAIL(buffer) (((buffer)->head + (((buffer)->head < (buffer)->tail) ? (buffer)->size : 0)) - (buffer)->tail)
#define FIFO_WRITE_AVAIL(buffer) (((buffer)->size - 1) - (((buffer)->head + (((buffer)->head < (buffer)->tail) ? (buffer)->size : 0)) - (buffer)->tail))

#define load_sym(V, S) do {\
    if (!((*(void**)&V) = SDL_LoadFunction(g_retro.handle, #S))) \
        die("Failed to load symbol '" #S "'': %s", SDL_GetError()); \
	} while (0)
#define load_retro_sym(S) load_sym(g_retro.S, S)

#define SNDBUFFLEN 8192 //2*2*2*1024
enum
{
	TEXT_LEFT,
	TEXT_CENTER,
	TEXT_RIGHT
};

enum {
	WG_BUTTON,
	WG_SELECT
};
