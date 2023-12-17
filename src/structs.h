typedef void (*Ptr_func)(void);

typedef struct {
	void (*logic)(void);
	void (*draw)(void);
	void (*init)(void);
} Delegate;

typedef struct {
     char * buf;
     int head;
     int tail;
     int size;
} fifo_t;

typedef struct Widget Widget;

struct Widget {
	int type;
	char name[MAX_NAME_LENGTH];
	int x;
	int y;
	int extra;
	char label[MAX_NAME_LENGTH];
	Widget *prev;
	Widget *next;
	void (*action)(void);
	void (*data);
};

typedef struct {
	int nb;
	char **subopt;
	int current;
}SubOpt;

typedef struct  {
	char name[MAX_OPTNAME_LENGTH];	
	char values[MAX_LINE_LENGTH];
	SubOpt sub;
}Coption;

typedef struct {
	int x;
	int y;
	int xr;
	int yr;
	int lb;
	int rb;
	int wu;
	int wd;
	int mb;
	int hwu;
	int hwd;
} Mouse;

struct keymap {
	unsigned k;
	unsigned rk;
};

typedef struct {	
	GLuint texture;
	int pitch;
	int tex_w, tex_h;
	int  clip_w, clip_h;
	GLuint pixfmt;
	GLuint pixtype;
	int  bpp;
} G_video;

typedef struct {
	void *handle;
	bool initialized;
	bool supports_no_game;
	
	void (*retro_init)(void);
	void (*retro_deinit)(void);
	unsigned (*retro_api_version)(void);
	void (*retro_get_system_info)(struct retro_system_info *info);
	void (*retro_get_system_av_info)(struct retro_system_av_info *info);
	void (*retro_set_controller_port_device)(unsigned port, unsigned device);
	void (*retro_reset)(void);
	void (*retro_run)(void);
	size_t (*retro_serialize_size)(void);
	bool (*retro_serialize)(void *data, size_t size);
	bool (*retro_unserialize)(const void *data, size_t size);
//	void retro_cheat_reset(void);
//	void retro_cheat_set(unsigned index, bool enabled, const char *code);
	bool (*retro_load_game)(const struct retro_game_info *game);
//	bool retro_load_game_special(unsigned game_type, const struct retro_game_info *info, size_t num_info);
	void (*retro_unload_game)(void);
//	unsigned retro_get_region(void);
//	void *retro_get_memory_data(unsigned id);
//	size_t retro_get_memory_size(unsigned id);
} G_retro;

