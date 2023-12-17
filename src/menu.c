#include "common.h"

extern Widget widgetHead, *widgetTail;
extern int SCREEN_W,SCREEN_H;
extern Uint32 fps_current;
extern Delegate delegate;
extern Widget *activeWidget;
extern int keyboard[MAX_KEYBOARD_KEYS];
extern Coption *coreopt;
extern int nb_coreopt;
extern bool coreoptupdate;
extern char *shader_name[];
extern int current_shader;

extern struct retro_variable *g_vars ;

static int LIMITE=0,FSELECT=0,PSELECT=0,SELECT=0,DEFAULT_OPT=0,PAS=10;

int SHOW_FPS=0;

char *showfps_name[] = { 
  "show fps: off",
  "show fps: on"
};

void menu_change_opt()
{
	int val=activeWidget->extra;	
//	printf("opt[%s]=%s\n",coreopt[val].name,coreopt[val].sub.subopt[coreopt[val].sub.current]);
	sprintf((char*)g_vars[val].value,"%s\0",coreopt[val].sub.subopt[coreopt[val].sub.current]);	
	
	coreoptupdate=true;
}


void menu_toggle_shaders()
{
	current_shader++;
	if(current_shader>MAX_SHADERS-1)current_shader=0;
	STRNCPY(activeWidget->label,shader_name[current_shader] ,31);
	printf("current:%d \n",current_shader);
	change_shaders();
}

void menu_toggle_fps()
{
	SHOW_FPS = !SHOW_FPS;	
	STRNCPY(activeWidget->label,showfps_name[SHOW_FPS] ,31);
}

void initMenu(void)
{
	Widget *w;
	int x;
	int ybase=16;
	
	x = 32;

	w = createWidget("resume");
	w->x = x;
	w->y = ybase;
	STRNCPY(w->label, "Resume",31);
	w->action = emulation_pause;
	w->type = WG_BUTTON;
	
	activeWidget = w;
	
	DEFAULT_OPT++;

	w = createWidget("reset");
	w->x = x;
	w->y = ybase*2;
	STRNCPY(w->label, "Reset core",31);
	w->action = core_reset;
	w->type = WG_BUTTON;
	
	DEFAULT_OPT++;
	
	w = createWidget("exit");
	w->x = x;
	w->y = ybase*3;
	STRNCPY(w->label, "Exit",31);
	w->action = emulation_quit;
	w->type = WG_BUTTON;

	DEFAULT_OPT++;

	w = createWidget("fps");
	w->x = x;
	w->y = ybase*4;
	STRNCPY(w->label,showfps_name[SHOW_FPS] ,31);
	w->action = menu_toggle_fps;
	w->type = WG_BUTTON;
	
	DEFAULT_OPT++;
		
	w = createWidget("shaders");
	w->x = x;
	w->y = ybase*5;
	STRNCPY(w->label,shader_name[current_shader] ,31);
	w->action = menu_toggle_shaders;
	w->type = WG_BUTTON;

	DEFAULT_OPT++;
	
	ybase=ybase*(DEFAULT_OPT+1);
	
	for(int i=0;i<nb_coreopt;i++){

		char tmp[32];
		sprintf(tmp,"options%2d",i);
		
		w = createWidget(tmp);
		w->x = x;
		w->y = ybase +i*16;
		w->extra = i;
		STRNCPY(w->label, coreopt[w->extra].name,31);
		w->action = menu_change_opt;
		w->type = WG_SELECT;
	}

	if(nb_coreopt+DEFAULT_OPT>MAX_ENTRIES_PER_PAGE)
		LIMITE=MAX_ENTRIES_PER_PAGE;
	else 
		LIMITE=nb_coreopt+DEFAULT_OPT;
	
}

static void changeWidgetValue(int dir)
{
	int val=0;

	switch (activeWidget->type)
	{
		case WG_SELECT:
			
			val=activeWidget->extra;
			
			coreopt[val].sub.current += dir;

			if (coreopt[val].sub.current < 0)
			{
				coreopt[val].sub.current = coreopt[val].sub.nb-1;
			}

			if (coreopt[val].sub.current >=  coreopt[val].sub.nb)
			{
				coreopt[val].sub.current = 0;
			}

			if (activeWidget->action != NULL)
			{
				activeWidget->action();
			}

			break;

		default:
			break;
	}
}

static void scroll_up(){

	SELECT--;
	if(SELECT<0){
		SELECT=nb_coreopt-1+DEFAULT_OPT;
		FSELECT=LIMITE-1;	
		PSELECT=SELECT-FSELECT;
	}
	else {
		if(FSELECT>0)FSELECT--;
		if(SELECT<PSELECT)PSELECT--;
	}
	
	activeWidget = activeWidget->prev;

	if (activeWidget == &widgetHead)
	{
		activeWidget = widgetTail;
	}	

}

static void scroll_down(){

	SELECT++;
	if(SELECT>nb_coreopt-1+DEFAULT_OPT){
		SELECT=PSELECT=FSELECT=0;
	}
	else {
		if(FSELECT<LIMITE-1)FSELECT++;
		if(SELECT> (PSELECT+LIMITE-1))PSELECT++;
	}	
			
	activeWidget = activeWidget->next;

	if (activeWidget == NULL)
	{
		activeWidget = widgetHead.next;
	}
	
}

void doWidgets(void)
{
	
	if (keyboard[SDL_SCANCODE_UP])
	{
		keyboard[SDL_SCANCODE_UP] = 0;

		scroll_up();
	}

	if (keyboard[SDL_SCANCODE_DOWN])
	{
		keyboard[SDL_SCANCODE_DOWN] = 0;
		
		scroll_down();
	}
					
	if (keyboard[SDL_SCANCODE_PAGEUP])
	{
		keyboard[SDL_SCANCODE_PAGEUP] = 0;

		for(int i=0;i<PAS;i++)	
			scroll_up();
	}

	if (keyboard[SDL_SCANCODE_PAGEDOWN])
	{
		keyboard[SDL_SCANCODE_PAGEDOWN] = 0;

		for(int i=0;i<PAS;i++)	
			scroll_down();
	}
	
	if (keyboard[SDL_SCANCODE_LEFT])
	{
		keyboard[SDL_SCANCODE_LEFT] = 0;

		changeWidgetValue(-1);
	}

	if (keyboard[SDL_SCANCODE_RIGHT])
	{
		keyboard[SDL_SCANCODE_RIGHT] = 0;

		changeWidgetValue(1);
	}
	
	if (keyboard[SDL_SCANCODE_RETURN])
	{
		keyboard[SDL_SCANCODE_RETURN] = 0;
		
		if (activeWidget->action != NULL)
		{
			activeWidget->action();
		}
	}
}

void drawWidgets(void)
{
	Widget *w;
	SDL_Color c;
	int i =0;
	
	int basey=0;
	
	for (w = widgetHead.next ; w != NULL ; w = w->next)
	{ 

	   	if(i>=PSELECT && i<PSELECT+ LIMITE){
		
			basey= ((i-PSELECT)<<4)+16;

			if (w == activeWidget)
			{
				c.g = 255;
				c.r = c.b = 0;
	
				drawText( w->x - 16, basey, c.r, c.g, c.b, TEXT_LEFT,"%c",14);
			}
			else
			{
					c.r = c.g = c.b = 255;
			}
		
			//if( i-PSELECT==FSELECT ) drawText( 0, basey, 255, 0, 0, TEXT_LEFT,"%c",5);
			drawText( w->x,basey, c.r, c.g, c.b, TEXT_LEFT, w->label);
		
			if(w->type==WG_SELECT){
				int val = w->extra;
				drawText( w->x+31*GLYPH_WIDTH, basey, c.r, c.g, c.b, TEXT_LEFT,\
				"< %s >",coreopt[val].sub.subopt[coreopt[val].sub.current] );
			}
				
	   	}
	   	i++;
	}
}


static void menu_init(void)
{
	//printf("enter menu\n");		
}

static void menu_logic(void)
{

	doWidgets();
	
	// Quit nanoarch when pressing the Escape key.
	if ( keyboard[SDL_SCANCODE_ESCAPE] == 1) {
		//printf("quit!!!\n");
		quit=true;
	}	
}

static void menu_draw(void)
{			
	video_render();
	
	if(SHOW_FPS)drawText(SCREEN_W-64, 32, 255, 255, 255, 0, "fps:%d",fps_current);
	
	drawWidgets();
}

void menu_setDelegate()
{
	delegate.init=menu_init;
	delegate.logic=menu_logic;
	delegate.draw=menu_draw;
}

