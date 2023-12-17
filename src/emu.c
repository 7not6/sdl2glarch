#include "common.h"

extern SDL_Window* window;
extern Delegate delegate;
extern int keyboard[MAX_KEYBOARD_KEYS];  
extern int SCREEN_W,SCREEN_H;
extern int SHOW_FPS;
extern Uint32 fps_current;

int PAUSE=-1;

static void emulation_init(void)
{
	//printf("enter emulation\n");
}

static void emulation_logic(void)
{
	// Reset core on R key.
	if (keyboard[SDL_SCANCODE_F12] == 1) {
		core_reset();
	}
	
	// toggle menu.
	if (keyboard[SDL_SCANCODE_ESCAPE] == 1 && PAUSE==-1) {
		keyboard[SDL_SCANCODE_ESCAPE]=0;
		emulation_pause();
	}	
	
	core_run();
}

static void emulation_draw(void)
{		
	video_render();
		
	if(SHOW_FPS)drawText(SCREEN_W-64, 32, 255, 255, 255, 0, "fps:%d",fps_current);	
}

void emulation_setDelegate(){

	delegate.init=emulation_init;
	delegate.logic=emulation_logic;
	delegate.draw=emulation_draw;
}

void emulation_pause()
{
	PAUSE=-PAUSE;
	
	if(PAUSE==-1){	

		audio_start();
		emulation_setDelegate();	
	}
	else {	
	
		audio_stop();
		menu_setDelegate();	
	}

	delegate.init();
}

void emulation_start(){		

	init_keyboards();
	
	emulation_setDelegate();
		
	delegate.init();

	initFonts();
	initWidgets();
	initMenu();
}

void emulation_quit()
{
	quit=true;
}

void emulation_cleanup(void)
{
	printf("sdl2arch quit!\n");
	
	core_unload();
	audio_deinit();
	video_deinit();
	deinitGL();
	deinitFonts();
	
	options_deinit();
	
	cleanWidgets();
		
	SDL_Quit(); 
}
