#include "common.h"

extern SDL_Window* window;

Mouse mouse;

int keyboard[MAX_KEYBOARD_KEYS];  //scancode
int keyboard2[MAX_KEYBOARD_KEYS]; //sdlk
int g_mouse[9]={0,0,0,0,0,0,0,0,0};


static void doKeyUp(SDL_KeyboardEvent *event)
{
	if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS)
	{
		keyboard[event->keysym.scancode] = 0;
		unsigned int tmp=(event->keysym.sym>0x40000000)?event->keysym.sym-0x40000000-0x39+128:event->keysym.sym;
		if(tmp<MAXK)keyboard2[tmp] = 0;
	}
}

static void doKeyDown(SDL_KeyboardEvent *event)
{
	if (event->repeat == 0 && event->keysym.scancode < MAX_KEYBOARD_KEYS)
	{
		keyboard[event->keysym.scancode] = 1;
		unsigned int tmp=(event->keysym.sym>0x40000000)?event->keysym.sym-0x40000000-0x39+128:event->keysym.sym;
		//printf("key:%d tmp:%d\n",event->keysym.sym,tmp);
		if(tmp<MAXK)keyboard2[tmp] = 1;
	}
}

static void mousePress(SDL_MouseButtonEvent b)
{
	if (b.button == SDL_BUTTON_RIGHT)mouse.rb=1;
	else if (b.button == SDL_BUTTON_LEFT)mouse.lb=1;
	else if (b.button == SDL_BUTTON_MIDDLE)mouse.mb=1;

}

static void mouseRelease(SDL_MouseButtonEvent b)
{
	if (b.button == SDL_BUTTON_RIGHT)mouse.rb=0;
	else if (b.button == SDL_BUTTON_LEFT)mouse.lb=0;
	else if (b.button == SDL_BUTTON_MIDDLE)mouse.mb=0;
}

int doInput(void)
{
	SDL_Event event;

	SDL_GetMouseState(&mouse.x, &mouse.y);
	SDL_GetRelativeMouseState(&mouse.xr, &mouse.yr);
	mouse.wd=mouse.wu=mouse.hwd=mouse.hwu=0;
	
	while (SDL_PollEvent(&event))
	{
		switch (event.type)
		{
			case SDL_QUIT:
				quit=true;
				break;
			case SDL_WINDOWEVENT:
                		switch (event.window.event) {
                			case SDL_WINDOWEVENT_CLOSE: quit=true; break;
                			case SDL_WINDOWEVENT_RESIZED:
                    			resize_cb(event.window.data1, event.window.data2);
                    			break;
                		}            		
            			
			case SDL_KEYDOWN:
				doKeyDown(&event.key);
				break;
				
			case SDL_KEYUP:
				doKeyUp(&event.key);
				break;
				
			case SDL_TEXTINPUT:
				//STRNCPY(inputText, event.text.text, MAX_LINE_LENGTH);
				break;
			case SDL_MOUSEWHEEL:
				if(event.wheel.y > 0)mouse.wu=event.wheel.y; // scroll up
			        else if(event.wheel.y < 0)mouse.wd=-event.wheel.y; // scroll down        			
        			if(event.wheel.x > 0) mouse.hwu=event.wheel.x;// scroll right
        		        else if(event.wheel.x < 0)mouse.hwd=-event.wheel.x; // scroll left
    
				break;
			case SDL_MOUSEBUTTONDOWN:
			 	mousePress(event.button);
				break;
			case SDL_MOUSEBUTTONUP:	
				mouseRelease(event.button);
				break;	
				
			default:
				break;
		}
	}
	return 0;	
	
}

void init_keyboards(){
	
	for(int i=0;i<MAX_KEYBOARD_KEYS;i++)
	{
		keyboard[i]=keyboard2[i]=0;	
	}
}
