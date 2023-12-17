#include "common.h"

SDL_GLContext *g_ctx = NULL;

extern float g_scale;

static G_video g_video  = {0};

SDL_Window* window = NULL;

int SCREEN_W,SCREEN_H;
int TEX_W,TEX_H;
int TEXC_W,TEXC_H;

void video_setgeometry(int w,int h){
    
	g_video.clip_w = w;
    g_video.clip_h = h;  
		
	TEX_W=g_video.tex_w;
	TEX_H=g_video.tex_h;
	TEXC_W=g_video.clip_w;
	TEXC_H=g_video.clip_h;  		
}

void prepareScene(void)
{
	glClearColor(0.f,0.f,0.f,1.f);
	glClear(GL_COLOR_BUFFER_BIT);
}

void presentScene(void)
{
	SDL_GL_SwapWindow(window);
}

void resize_cb(int w, int h) {
	
	glViewport(0, 0, w, h);
	
	refresh_vertex_data();
	SCREEN_W=w;
	SCREEN_H=h;	
//	printf("size:%dx%d clip:%dx%d\n",SCREEN_W,SCREEN_H,g_video.clip_w ,g_video.clip_h);	
	
	TEX_W=g_video.tex_w;
	TEX_H=g_video.tex_h;
	TEXC_W=g_video.clip_w;
	TEXC_H=g_video.clip_h;  
	
}

static void create_window(int width, int height) {
	
	int windowFlags;

	SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
       
        SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    	
        SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
   	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
   	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
        SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
	
	windowFlags =  SDL_WINDOW_RESIZABLE|SDL_WINDOW_OPENGL;
	
	window = SDL_CreateWindow("sdl2arch", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, width, height, windowFlags);
		
								
	if (!window)
		 die("Failed to create window: %s", SDL_GetError());

	int w,h;
	SDL_GetWindowSize(window,&w,&h);
	
	printf("Create windows size:%dx%d scale:%f\n",w,h,g_scale);

	SDL_SetWindowFullscreen( window,0);
	
	g_ctx = SDL_GL_CreateContext(window);
    	SDL_GL_MakeCurrent(window, g_ctx);
	
	if (!initGL()) {
		die("Failed to init GL!");
	}

	SDL_GL_SetSwapInterval(1);
	SDL_GL_SwapWindow(window); // make apitrace output nicer
    
	resize_cb(width, height);
		
}

void video_configure(const struct retro_game_geometry *geom) {
	int nwidth, nheight;

	resize_to_aspect(geom->aspect_ratio, geom->base_width * 1, geom->base_height * 1, &nwidth, &nheight);

	nwidth *= g_scale;
	nheight *= g_scale;

	if (!window)
		create_window(nwidth, nheight);

	if (g_video.texture)
		glDeleteTextures(1, &g_video.texture);

	g_video.texture = 0;

	if (!g_video.pixfmt)
		g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
		
        SDL_SetWindowSize(window, nwidth, nheight);
        
 	glGenTextures(1, &g_video.texture);

	if (!g_video.texture)
		die("Failed to create the video texture");

	g_video.pitch = geom->base_width * g_video.bpp;

	glBindTexture(GL_TEXTURE_2D, g_video.texture);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, geom->max_width, geom->max_height, 0,
			g_video.pixtype, g_video.pixfmt, NULL);

	glBindTexture(GL_TEXTURE_2D, 0);
	
	g_video.tex_w = geom->max_width;
	g_video.tex_h = geom->max_height;
	g_video.clip_w = geom->base_width;
	g_video.clip_h = geom->base_height;

	TEX_W=g_video.tex_w ;
	TEX_H=g_video.tex_h;
	TEXC_W=g_video.clip_w;
	TEXC_H=g_video.clip_h;
	
	refresh_vertex_data();
}


void resize_to_aspect(double ratio, int sw, int sh, int *dw, int *dh) {

	printf("r:%f sw:%d sh:%d ",ratio,sw,sh);
	*dw = sw;
	*dh = sh;

	if (ratio <= 0)
		ratio = (double)sw / sh;

	if ((float)sw / sh < 1)
		*dw = (int)roundf((float)(*dh * ratio));
	else
		*dh = (int)roundf((float)(*dw / ratio)); 
 
#if 0
	 if ((float)sw / sh < 1)
		*dw = *dh * ratio;
	else
		*dh = *dw / ratio;
#endif	 			
	//printf("dw:%d dh:%d\n",*dw,*dh);
}


bool video_set_pixel_format(unsigned format) {

	switch (format) {
	case RETRO_PIXEL_FORMAT_0RGB1555:
		g_video.pixfmt = GL_UNSIGNED_SHORT_5_5_5_1;
		g_video.bpp = sizeof(uint16_t);
		g_video.pixtype = GL_BGRA;
		break;
	case RETRO_PIXEL_FORMAT_XRGB8888:
		g_video.pixfmt = GL_UNSIGNED_INT_8_8_8_8_REV;
		g_video.bpp = sizeof(uint32_t);
		g_video.pixtype = GL_BGRA;
		break;
	case RETRO_PIXEL_FORMAT_RGB565:
		g_video.pixfmt  = GL_UNSIGNED_SHORT_5_6_5;
		g_video.bpp = sizeof(uint16_t);
		g_video.pixtype = GL_RGB;
		break;
	default:
		die("Unknown pixel type %u", format);
	}

	return true;
}


void video_refresh(const void *data, unsigned width, unsigned height, unsigned pitch) {
	
	if (g_video.clip_w != width || g_video.clip_h != height) {
		g_video.clip_h = height;
		g_video.clip_w = width;
		//refresh_vertex_data();
		printf("rrrrrr\n");		
	}
	if (pitch != g_video.pitch) {
		g_video.pitch = pitch;
		//printf("change pitch:%d\n",pitch);
		//printf("refresh t(%d,%d) c(%d,%d)\n",g_video.tex_w,g_video.tex_h,g_video.clip_w,g_video.clip_h);
	}

	if (data) {
		glBindTexture(GL_TEXTURE_2D, g_video.texture);
		//printf("%d %d %d %d %d \n",width,  height,  pitch,g_video.pixtype, g_video.pixfmt);	
		glPixelStorei(GL_UNPACK_ROW_LENGTH, g_video.pitch / g_video.bpp);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
						g_video.pixtype, g_video.pixfmt, data);
	}		
}

void video_render() {

	drawTextureGL(g_video.texture);
}

void video_deinit() {

		if (g_video.texture)
		glDeleteTextures(1, &g_video.texture);
		       
		g_video.texture = 0;
	
		SDL_GL_MakeCurrent(window, g_ctx);
    	SDL_GL_DeleteContext(g_ctx);

    	g_ctx = NULL;
    	SDL_DestroyWindow(window);
    		   
}
