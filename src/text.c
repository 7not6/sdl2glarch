#include "common.h"
#include "font10x16.h"

static char drawTextBuffer[MAX_LINE_LENGTH];
static GLuint fontTexture;
float UV[FONT_BOXW*FONT_BOXH][4];

void setUVFonts(){

	int i,j,idx=0;	
	float sx,sy;
	
	for(j=0;j<FONT_BOXH;j++){
	
		for(i=0;i<FONT_BOXW;i++){
		
			sx=(float)(i*GLYPH_WIDTH);
			sy=(float)(j*GLYPH_HEIGHT);
			idx=i+j*FONT_BOXW;
			UV[idx][0]=(float)sx/(float)FONT_WIDTH;
			UV[idx][1]=(float)(sx+(float)GLYPH_WIDTH)/(float)FONT_WIDTH;
			UV[idx][2]=(float)sy/(float)FONT_HEIGHT;
			UV[idx][3]=(float)(sy+(float)GLYPH_HEIGHT)/(float)FONT_HEIGHT; 				
		}		
	}
}
	
void initFonts(void)
{
	SDL_LogMessage(SDL_LOG_CATEGORY_APPLICATION, SDL_LOG_PRIORITY_INFO, "init font10x16 texture" );	
	
	SDL_Surface *tmp = NULL;
	unsigned char tmpfont[FONT10X16_WIDTH*FONT10X16_HEIGHT*4];
	
	int i,j,n,ind=0;
	Uint32 *ptr=(Uint32*)tmpfont;
	unsigned char binary[8];
	
	for(j=0;j<FONT10X16_HEIGHT;j++){
	
		for(i=0;i<FONT10X16_WIDTH;i+=8){
		
			unsigned char val=font10x16[ind++];
			
			for(n = 0; n < 8; n++)
			 	binary[7-n] = (val >> n) & 1;
			 	
			for(n = 0; n < 8; n++){	
				if(binary[n] == 1)*ptr=0xffffffff;
				else *ptr=0;	
				ptr++;
			} 						
		}		
	}
	
	tmp = SDL_CreateRGBSurfaceFrom((void *)tmpfont, FONT10X16_WIDTH, FONT10X16_HEIGHT, 32,FONT10X16_WIDTH*4, 0xff000000, 0x00ff0000, 0x0000ff00,0x000000ff);
	
	if(NULL == tmp){
		fprintf(stderr, "Error SDL_CreateRGBSurface tmp: %s", SDL_GetError());	
		goto quit;
	}
		
        glGenTextures(1, &fontTexture);

	if (!fontTexture)
		die("Failed to create the font texture");
		
	glBindTexture(GL_TEXTURE_2D, fontTexture);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, FONT10X16_WIDTH,FONT10X16_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_INT_8_8_8_8_REV, tmp->pixels);

	glBindTexture(GL_TEXTURE_2D, 0);
	
	SDL_FreeSurface(tmp);

	setUVFonts();
	
	quit:

	if (!fontTexture)printf("create font failed\n");
}

void deinitFonts(void)
{
	if (fontTexture)
		glDeleteTextures(1, &fontTexture);
}
	
void drawChar(int x, int y, int r, int g, int b, char c)
{
	
	int cy=c/GLYPH_HEIGHT;
	int cx=c-cy*GLYPH_HEIGHT;
	int idx= cx + cy*FONT_BOXW;

	SetFontColorGL( r,  g,  b);				
	blitFontGL(fontTexture, idx, x, y,1.0f);
	RestorePrgGL();
}

void drawCharScale(int x, int y, int r, int g, int b, char c,int scale)
{

	int cy=c/GLYPH_HEIGHT;
	int cx=c-cy*GLYPH_HEIGHT;
	int idx= cx + cy*FONT_BOXW;
	
	SetFontColorGL( r,  g,  b);				
	blitFontGL(fontTexture, idx, x, y,scale);
	RestorePrgGL();
}

void drawText(int x, int y, int r, int g, int b, int align,char *format, ...)
{
	int i, len, c;
	va_list args;
	
	memset(&drawTextBuffer, '\0', sizeof(drawTextBuffer));

	va_start(args, format);
	vsprintf(drawTextBuffer, format, args);
	va_end(args);
	
	len = strlen(drawTextBuffer);
	
	switch (align)
	{
		case TEXT_RIGHT:
			x -= (len * GLYPH_WIDTH);
			break;
			
		case TEXT_CENTER:
			x -= (len * GLYPH_WIDTH) / 2;
			break;
	}

	SetFontColorGL( r,  g,  b);
		
	for (i = 0 ; i < len ; i++)
	{
		c = drawTextBuffer[i];
		
		int cy=c/GLYPH_HEIGHT;
		int cx=c-cy*GLYPH_HEIGHT;
		int idx= cx + cy*FONT_BOXW;

		blitFontGL(fontTexture, idx, x, y,1.0f);
			
		x += GLYPH_WIDTH;
	}
	
	RestorePrgGL();
}

void drawTextScale(int x, int y, int r, int g, int b, int align,int scale,char *format, ...)
{
	int i, len, c;

	va_list args;
	
	memset(&drawTextBuffer, '\0', sizeof(drawTextBuffer));

	va_start(args, format);
	vsprintf(drawTextBuffer, format, args);
	va_end(args);
	
	len = strlen(drawTextBuffer);
	
	switch (align)
	{
		case TEXT_RIGHT:
			x -= (len * GLYPH_WIDTH);
			break;
			
		case TEXT_CENTER:
			x -= (len * GLYPH_WIDTH) / 2;
			break;
	}

	SetFontColorGL( r,  g,  b);
			
	for (i = 0 ; i < len ; i++)
	{
		c = drawTextBuffer[i];
		
		int cy=c/GLYPH_HEIGHT;
		int cx=c-cy*GLYPH_HEIGHT;
		int idx= cx + cy*FONT_BOXW;				

		blitFontGL(fontTexture, idx, x, y,scale);
		x += GLYPH_WIDTH*scale;
	}
	
	RestorePrgGL();	
}
