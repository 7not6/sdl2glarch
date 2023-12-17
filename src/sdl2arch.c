#include "common.h"
	
Delegate delegate;

bool quit = false;
Uint32 fps_current=0;
char *savestated = NULL;

int main(int argc, char *argv[]) {

	Uint32 fps_lasttime = SDL_GetTicks(); 
	Uint32 fps_frames = 0; 
	
	if (argc < 2)
		die("usage: %s <core> <game> [-s default-scale] [-l load-savestate] [-d save-savestate]", argv[0]);

	if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0)
   	{
       		fprintf(stderr, "SDL_Init Error: %s\n", SDL_GetError());
       		exit(1);
    	}
	
	atexit(emulation_cleanup);

	core_main(argc,argv);
	
	emulation_start();
	
	while (!quit) {
		     
		doInput();  
				 
		delegate.logic();
		
		prepareScene();		
		delegate.draw();
		presentScene();
			
		fps_frames++;
   		if (fps_lasttime < SDL_GetTicks() - FPS_INTERVAL*1000)
   		{
      			fps_lasttime = SDL_GetTicks();
      			fps_current = fps_frames;
      			fps_frames = 0;      			
   		}
	  	
	}

	if (savestated)save_savestate(savestated);	
    
    return EXIT_SUCCESS;
}
