#include "common.h"

#include "keyb.h"

extern SDL_Window* window;
extern Mouse mouse;

extern Coption *coreopt;
extern int nb_coreopt;
extern bool coreoptupdate;

extern int keyboard[MAX_KEYBOARD_KEYS];  
extern int keyboard2[MAX_KEYBOARD_KEYS]; 
extern int g_mouse[9];
extern char *savestated;
extern int PAUSE;

float g_scale = 2;
static unsigned g_joy[RETRO_DEVICE_ID_JOYPAD_R3+1] = { 0 };
static unsigned g_key[MAXK]={0};

static G_retro g_retro;


struct retro_variable *g_vars = NULL;
struct retro_audio_callback audio_callback;

void core_reset()
{
	if(PAUSE==1)emulation_pause();
	g_retro.retro_reset();
}

void core_run()
{
	// Ask the core to emit the audio.
        if (audio_callback.callback) {
          	audio_callback.callback();
        }

	g_retro.retro_run();
}

static void core_log(enum retro_log_level level, const char *fmt, ...) {
	char buffer[4096] = {0};
	static const char * levelstr[] = { "dbg", "inf", "wrn", "err" };
	va_list va;

	va_start(va, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, va);
	va_end(va);

	if (level == 0)
		return;

	fprintf(stderr, "[%s] %s", levelstr[level], buffer);
	fflush(stderr);

	if (level == RETRO_LOG_ERROR)
		exit(EXIT_FAILURE);
}

static bool core_environment(unsigned cmd, void *data) {
	bool *bval;

	switch (cmd) {
	
	 case RETRO_ENVIRONMENT_SET_VARIABLES: {
        const struct retro_variable *vars = (const struct retro_variable *)data;
        size_t num_vars = 0;

        for (const struct retro_variable *v = vars; v->key; ++v) {
            num_vars++;
        }
        
        nb_coreopt=num_vars;
        coreopt=malloc(sizeof(Coption)*nb_coreopt);

        g_vars = (struct retro_variable*)calloc(num_vars + 1, sizeof(*g_vars));
        for (unsigned i = 0; i < num_vars; ++i) {
            const struct retro_variable *invar = &vars[i];
            struct retro_variable *outvar = &g_vars[i];

            const char *semicolon = strchr(invar->value, ';');
            const char *first_pipe = strchr(invar->value, '|');

            SDL_assert(semicolon && *semicolon);
            semicolon++;
            while (isspace(*semicolon))
                semicolon++;

            if (first_pipe) {
                outvar->value = malloc((first_pipe - semicolon) + 1);
                memcpy((char*)outvar->value, semicolon, first_pipe - semicolon);
                ((char*)outvar->value)[first_pipe - semicolon] = '\0';
            } else {
                outvar->value = strdup(semicolon);
            }
            
	    sprintf(coreopt[i].name,"%s\0",invar->key);
	    size_t tmp=strlen(semicolon)+1;
	    if(tmp>=MAX_LINE_LENGTH)tmp=MAX_LINE_LENGTH;
	    memcpy((char*)coreopt[i].values , semicolon,tmp);
	    
	    split(coreopt[i].values,"|",i);	    	    
 
            outvar->key = strdup(invar->key);
            SDL_assert(outvar->key && outvar->value);
	   // SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "0(%s)(%s)\n",invar->key,coreopt[i].values);
	
        }

        return true;
       }
        case RETRO_ENVIRONMENT_GET_VARIABLE: {
        struct retro_variable *var = (struct retro_variable *)data;

        if (!g_vars)
            return false;

        for (const struct retro_variable *v = g_vars; v->key; ++v) {
			//printf("1)%s=%s\n", var->key , var->value);
            if (strcmp(var->key, v->key) == 0) {
                var->value = v->value;
                break;
            }
        }

        return true;
    }
    case RETRO_ENVIRONMENT_GET_VARIABLE_UPDATE: {
        bool *bval = (bool*)data;
		*bval = false;
		*bval = coreoptupdate;
		if(coreoptupdate==true){
			coreoptupdate=false;
			printf("ok\n");
		}
		
        return true;
    }
	case RETRO_ENVIRONMENT_GET_LOG_INTERFACE: {
		struct retro_log_callback *cb = (struct retro_log_callback *)data;
		cb->log = core_log;
        return true;
	}	

	case RETRO_ENVIRONMENT_GET_CAN_DUPE:
		bval = (bool*)data;
		*bval = true;
		break;
	case RETRO_ENVIRONMENT_SET_PIXEL_FORMAT: {
		const enum retro_pixel_format *fmt = (enum retro_pixel_format *)data;

		if (*fmt > RETRO_PIXEL_FORMAT_RGB565)
			return false;

		return video_set_pixel_format(*fmt);
	}
	case RETRO_ENVIRONMENT_GET_SYSTEM_DIRECTORY:
	case RETRO_ENVIRONMENT_GET_SAVE_DIRECTORY:
		*(const char **)data = ".";
		return true;

    case RETRO_ENVIRONMENT_SET_GEOMETRY: {
        const struct retro_game_geometry *geom = (const struct retro_game_geometry *)data;
        video_setgeometry(geom->base_width,geom->base_height);
        // some cores call this before we even have a window
        if (window) {
            refresh_vertex_data();
            int ow = 0, oh = 0;
            resize_to_aspect(geom->aspect_ratio, geom->base_width, geom->base_height, &ow, &oh);

            ow *= g_scale;
            oh *= g_scale;

            SDL_SetWindowSize(window, ow, oh);
        }
        return true;
    }
    
        case RETRO_ENVIRONMENT_SET_AUDIO_CALLBACK: {
        struct retro_audio_callback *audio_cb = (struct retro_audio_callback*)data;
        audio_callback = *audio_cb;
        return true;
    }
    
        case RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME: {
        g_retro.supports_no_game = *(bool*)data;
        return true;
    }
	default:
		core_log(RETRO_LOG_DEBUG, "Unhandled env #%u", cmd);
		return false;
	}

	return true;
}

static void core_video_refresh(const void *data, unsigned width, unsigned height, size_t pitch) {
	if (data)
		video_refresh(data, width, height, pitch);
}

static void core_input_poll(void) {
	int i;
	
	for (i = 0; g_binds[i].k || g_binds[i].rk; ++i)
		g_joy[g_binds[i].rk] = (keyboard[g_binds[i].k] == 1);

	//keyb
	for (i = 0; i<MAXK; ++i){
		unsigned int tmp=(g_kbinds[i].rk>0x40000000)?g_kbinds[i].rk-0x40000000-0x39+128:g_kbinds[i].rk;
		g_key[g_kbinds[i].k] = (keyboard2[tmp]==1);
	}	
	
	//mouse	
	g_mouse[0] =mouse.xr;
	g_mouse[1] =mouse.yr;
	g_mouse[2] =mouse.lb;
	g_mouse[3] =mouse.rb;
	g_mouse[4] =mouse.wu;
	g_mouse[5] =mouse.wd;
	g_mouse[6] =mouse.mb;
	g_mouse[7] =mouse.hwu;
	g_mouse[8] =mouse.hwd;	

}

static int16_t core_input_state(unsigned port, unsigned device, unsigned index, unsigned id) {

	if (port || index || (device != RETRO_DEVICE_JOYPAD && device != RETRO_DEVICE_KEYBOARD && device !=RETRO_DEVICE_MOUSE) )
		return 0;

	if(device ==RETRO_DEVICE_JOYPAD){
		return g_joy[id];
	}
	else if(device ==RETRO_DEVICE_KEYBOARD){
		return g_key[id];
	}
	
	else if(device ==RETRO_DEVICE_MOUSE){
		return g_mouse[id];	
	}
	
	return 0;
}

static void core_audio_sample(int16_t left, int16_t right) {
	int16_t buf[2] = {left, right};
	audio_write(buf, 1);
}


static size_t core_audio_sample_batch(const int16_t *data, size_t frames) {
	return audio_write(data, frames);
}


static void core_load(const char *sofile) {
	void (*set_environment)(retro_environment_t) = NULL;
	void (*set_video_refresh)(retro_video_refresh_t) = NULL;
	void (*set_input_poll)(retro_input_poll_t) = NULL;
	void (*set_input_state)(retro_input_state_t) = NULL;
	void (*set_audio_sample)(retro_audio_sample_t) = NULL;
	void (*set_audio_sample_batch)(retro_audio_sample_batch_t) = NULL;

	memset(&g_retro, 0, sizeof(g_retro));
	
        g_retro.handle = SDL_LoadObject(sofile);

	if (!g_retro.handle)
        die("Failed to load core: %s", SDL_GetError());

	load_retro_sym(retro_init);
	load_retro_sym(retro_deinit);
	load_retro_sym(retro_api_version);
	load_retro_sym(retro_get_system_info);
	load_retro_sym(retro_get_system_av_info);
	load_retro_sym(retro_set_controller_port_device);
	load_retro_sym(retro_reset);
	load_retro_sym(retro_run);
	load_retro_sym(retro_load_game);
	load_retro_sym(retro_unload_game);
	load_retro_sym(retro_serialize_size);
	load_retro_sym(retro_serialize);
	load_retro_sym(retro_unserialize);

	load_sym(set_environment, retro_set_environment);
	load_sym(set_video_refresh, retro_set_video_refresh);
	load_sym(set_input_poll, retro_set_input_poll);
	load_sym(set_input_state, retro_set_input_state);
	load_sym(set_audio_sample, retro_set_audio_sample);
	load_sym(set_audio_sample_batch, retro_set_audio_sample_batch);

	set_environment(core_environment);
	set_video_refresh(core_video_refresh);
	set_input_poll(core_input_poll);
	set_input_state(core_input_state);
	set_audio_sample(core_audio_sample);
	set_audio_sample_batch(core_audio_sample_batch);

	g_retro.retro_init();
	g_retro.initialized = true;

	puts("Core loaded");
}

static void core_load_game(const char *filename) {

	struct retro_system_av_info av = {0};
	struct retro_system_info system = {0};
	struct retro_game_info info = { filename, 0 };

    info.path = filename;
    info.meta = "";
    info.data = NULL;
    info.size = 0;
    
    g_retro.retro_get_system_info(&system);
    
    if (filename) {
 
        if (!system.need_fullpath) {
            SDL_RWops *file = SDL_RWFromFile(filename, "rb");
            Sint64 size;

            if (!file)
                die("Failed to load %s: %s", filename, SDL_GetError());

            size = SDL_RWsize(file);

            if (size < 0)
                die("Failed to query game file size: %s", SDL_GetError());

            info.size = size;
            info.data = SDL_malloc(info.size);

            if (!info.data)
                die("Failed to allocate memory for the content");

            if (!SDL_RWread(file, (void*)info.data, info.size, 1))
                die("Failed to read file data: %s", SDL_GetError());

            SDL_RWclose(file);
        }
    }

	if (!g_retro.retro_load_game(&info))
		die("The core failed to load the content.");

	g_retro.retro_get_system_av_info(&av);

	video_configure(&av.geometry);
	audio_init(av.timing.sample_rate);
	
	if((av.timing.sample_rate/av.timing.fps)>=SNDBUFFLEN){
		printf("maxsnd:%d\n",(int)(av.timing.sample_rate/av.timing.fps)*2);
		printf("maxsnd too big , exiting now!\n");
		exit(1);
	}
	
    if (info.data)
        SDL_free((void*)info.data);

    // Now that we have the system info, set the window title.
    char window_title[255];
    snprintf(window_title, sizeof(window_title), "sdl2arch %s %s", system.library_name, system.library_version);
    SDL_SetWindowTitle(window, window_title);
}

void core_unload() {
	if (g_retro.initialized)
		g_retro.retro_deinit();

	if (g_retro.handle)
		SDL_UnloadObject(g_retro.handle);
}

void load_savestate(const char *savestatel) {
	
	if (savestatel) {
		FILE *fd = fopen(savestatel, "rb");
		if (!fd)
			die("Failed to find savestate file '%s'", savestatel);

		void *saveblob = malloc(g_retro.retro_serialize_size());
		size_t rdb = fread(saveblob, 1, g_retro.retro_serialize_size(), fd);

		if (!g_retro.retro_unserialize(saveblob, rdb))
			die("Failed to load savestate, core returned error");
		fclose(fd);
		free(saveblob);
	}	
}

void save_savestate(const char *savestated) {
	if (savestated) {
		FILE *fd = fopen(savestated, "wb");
		if (!fd) {
			fprintf(stderr, "Could not write savestate dump to '%s'", savestated);
		} else {
			size_t savsz = g_retro.retro_serialize_size();
			void *saveblob = malloc(savsz);
			if (!g_retro.retro_serialize(saveblob, savsz)) {
				fprintf(stderr, "Could not generate savestate, core returned error");
			} else {
				fwrite(saveblob, 1, savsz, fd);
			}
			free(saveblob);
			fclose(fd);
		}
	}
	
}

void conf_player_input(){
	
	// Configure the player input devices.
	g_retro.retro_set_controller_port_device(0, RETRO_DEVICE_JOYPAD);
}

int core_main(int argc, char *argv[]) {
	
	char **opts = &argv[3];
	
	char *savestatel = NULL;
	
	while (*opts) {
		if (!strcmp(*opts, "-s"))
			g_scale = atoi(*(++opts));
		else if (!strcmp(*opts, "-l"))
			savestatel = *(++opts);
		else if (!strcmp(*opts, "-d"))
			savestated = *(++opts);
		opts++;
	}
	
	core_load(argv[1]);
	
	if (!g_retro.supports_no_game && argc < 3)
        	die("This core requires a game in order to run");

    	core_load_game(argc > 2 ? argv[2] : NULL);
       
	conf_player_input();      
    
	if(savestatel)load_savestate( savestatel);	

	return  0;
}

void options_deinit(){

	if (g_vars) {
		int i=0;
        	for (const struct retro_variable *v = g_vars; v->key; ++v,i++) {
        		free((char*)v->key);
            		free((char*)v->value);
            		
            		freesuboptval(i);            		
        	}
        	free(g_vars);
        	free(coreopt);
        }
}

