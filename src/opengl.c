#include "common.h"

extern int SCREEN_W,SCREEN_H;
extern int TEX_W,TEX_H;
extern int TEXC_W,TEXC_H;
extern bool shader_disable;
extern float UV[FONT_BOXW*FONT_BOXH][4];
extern struct retro_hw_render_callback hw;

int current_shader=0;

static GLint g_tex_uniform = 0;
static GLint u_texture_projection = 0;
static GLuint o_size = 0;
static GLuint t_size = 0;
static GLuint fcol = 0;
    
GLint i_pos;
GLint i_coord;
	
GLuint vao;
GLuint vbo;
GLuint font_vao;
GLuint font_vbo;
GLuint fbo;
GLuint rbo;

int glmajor;
int glminor;
	
GLfloat projection[4][4];
   	
static const char *g_vshader_src =
    "#version 150\n"
    "uniform mat4 u_projection;\n"
    "uniform vec2 OutputSize;\n"
    "uniform vec2 TextureSize;\n"
    "uniform vec4 Fontcolor;\n"
    "in vec2 i_pos;\n"
    "in vec2 i_coord;\n"
    "out vec2 o_coord;\n"
    "void main() {\n"
        "o_coord = i_coord;\n"
	"gl_Position = u_projection * vec4(i_pos, 0.0, 1.0);\n"
    "}";

static const char *g_fshader_font_src =
    "#version 150\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec4 Fontcolor;\n"
    "in vec2 o_coord;\n"
    "out vec4 Out_Color;\n"
    "void main() {\n"
    	"float alpha = texture(u_tex, o_coord).a;\n"
        "Out_Color = vec4(Fontcolor.xyz, alpha);\n"
    "}";
	
static const char *g_fshader_stock_src =
    "#version 150\n"
    "uniform sampler2D u_tex;\n"
    "in vec2 o_coord;\n"
    "out vec4 Out_Color;\n"
    "void main() {\n"
        "Out_Color =vec4(texture2D(u_tex, o_coord).rgba);\n"
    "}";	

static const char *g_fshader_retro_src =
    "#version 150\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 OutputSize;\n"
    "uniform vec2 TextureSize;\n"
    "#define BLURSCALEX 0.45\n"
    "#define LOWLUMSCAN 5.0 \n"
    "#define HILUMSCAN 10.0\n"
    "#define BRIGHTBOOST 1.25\n"
    "#define MASK_DARK 0.25\n"
    "#define MASK_FADE 0.8\n"
    "#define vTexCoord o_coord\n"
    "#define Source u_tex\n"
    "in vec2 o_coord;\n"
    "out vec4 Out_Color;\n"
    "void main() {\n"    
    	"float maskFade = 0.3333*MASK_FADE;\n"
  	"vec2 invDims = 1.0/TextureSize.xy;\n"
    	"vec2 p = vTexCoord * TextureSize;\n"
  	"vec2 i = floor(p) + 0.50;\n"
  	"vec2 f = p - i;\n"
	"p = (i + 4.0*f*f*f)*invDims;\n"
  	"p.x = mix( p.x , vTexCoord.x, BLURSCALEX);\n"
  	"float Y = f.y*f.y;\n"
  	"float YY = Y*Y;\n"
	"float whichmask = fract( gl_FragCoord.x*-0.4999);\n"
  	"float mask = 1.0 + float(whichmask < 0.5) * -MASK_DARK;\n"
	"vec3 colour = texture(Source, p).rgb;\n"
	"float scanLineWeight = (BRIGHTBOOST - LOWLUMSCAN*(Y - 2.05*YY));\n"
  	"float scanLineWeightB = 1.0 - HILUMSCAN*(YY-2.8*YY*Y);\n"
	"Out_Color = vec4(colour.rgb*mix(scanLineWeight*mask, scanLineWeightB, dot(colour.rgb,vec3(maskFade))), 1.0);\n"
    "}";

static const char *g_fshader_scanline_src =
    "#version 150\n"
    "float density = 1.3;\n"
    "float opacityScanline = .3;\n"
    "uniform sampler2D u_tex;\n"
    "uniform vec2 OutputSize;\n"
    "uniform vec2 TextureSize;\n"
    "in vec2 o_coord;\n"
    "out vec4 Out_Color;\n"    
    "void main() {\n"
    	"vec2 uv = o_coord;\n"
    	"vec3 col = texture(u_tex,uv).rgb;\n"
    	"float count = OutputSize.y * density;\n"
    	"vec2 sl = vec2(sin(uv.y * count), cos(uv.y * count));\n"
	"vec3 scanlines = vec3(sl.x, sl.y, sl.x);\n"
	"col += col * scanlines * opacityScanline;\n"
        "Out_Color =vec4(col,1.0);\n"
    "}";	

static const char *g_fshader_gray_src =
    "#version 150\n"
    "uniform sampler2D u_tex;\n"
    "in vec2 o_coord;\n"
    "out vec4 Out_Color;\n"
    "void main() {\n"
        "vec3 gammaColor = texture(u_tex, o_coord).rgb;\n"
        "vec3 color = pow(gammaColor, vec3(2.0));\n"
        "float gray = dot(color, vec3(0.2126, 0.7152, 0.0722));\n"
        "float gammaGray = sqrt(gray);\n"
        "Out_Color = vec4(gammaGray,gammaGray,gammaGray,1.0);\n"
    "}";

GLuint program_Id[MAX_SHADERS];
GLuint Program_fontId;
GLuint ProgramId;
GLint OldProgramId;

char *shader_name[] = { 
  "shaders: stock",
  "shaders: retro",
  "shaders: scanline",
  "shaders: gray"
};


PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLDELETEVERTEXARRAYSPROC glDeleteVertexArrays;
PFNGLDELETEBUFFERSPROC glDeleteBuffers;
PFNGLDELETEPROGRAMPROC glDeleteProgram;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLDISABLEVERTEXATTRIBARRAYPROC glDisableVertexAttribArray;
PFNGLUNIFORM1IPROC glUniform1i;
PFNGLUNIFORM2FPROC glUniform2f;
PFNGLUNIFORM4FPROC glUniform4f;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLBINDATTRIBLOCATIONPROC glBindAttribLocation;
PFNGLGETACTIVEUNIFORMPROC glGetActiveUniform;
PFNGLGETACTIVEATTRIBPROC glGetActiveAttrib;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLDELETEFRAMEBUFFERSPROC glDeleteFramebuffers;
PFNGLGENFRAMEBUFFERSPROC glGenFramebuffers;
PFNGLBINDFRAMEBUFFERPROC glBindFramebuffer;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D;
PFNGLGENRENDERBUFFERSPROC glGenRenderbuffers;
PFNGLBINDRENDERBUFFERPROC glBindRenderbuffer;
PFNGLRENDERBUFFERSTORAGEPROC glRenderbufferStorage;
PFNGLFRAMEBUFFERRENDERBUFFERPROC glFramebufferRenderbuffer;
PFNGLCHECKFRAMEBUFFERSTATUSPROC glCheckFramebufferStatus;
PFNGLDELETERENDERBUFFERSPROC glDeleteRenderbuffers;

bool initGLExtensions() {
	glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
	glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
	glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
	glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glGetAttribLocation");
        glGetUniformLocation =(PFNGLGETUNIFORMLOCATIONPROC) SDL_GL_GetProcAddress("glGetUniformLocation");
        glBindVertexArray =  (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
	glDeleteVertexArrays = (PFNGLDELETEVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glDeleteVertexArrays");
	glDeleteBuffers =(PFNGLDELETEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteBuffers");
	glDeleteProgram =(PFNGLDELETEPROGRAMPROC)SDL_GL_GetProcAddress("glDeleteProgram");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
	glGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	glBindBuffer =(PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	glBufferData =(PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer =	(PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
	glUniform1i = (PFNGLUNIFORM1IPROC)SDL_GL_GetProcAddress("glUniform1i");
	glDisableVertexAttribArray = (PFNGLDISABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glDisableVertexAttribArray");
	glUniform2f = (PFNGLUNIFORM2FPROC)SDL_GL_GetProcAddress("glUniform2f");
	glUniform4f = (PFNGLUNIFORM4FPROC)SDL_GL_GetProcAddress("glUniform4f");
	glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
	glBindAttribLocation = (PFNGLBINDATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glBindAttribLocation");
	glGetActiveUniform = (PFNGLGETACTIVEUNIFORMPROC)SDL_GL_GetProcAddress("glGetActiveUniform");
	glGetActiveAttrib = (PFNGLGETACTIVEATTRIBPROC)SDL_GL_GetProcAddress("glGetActiveAttrib");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
	glDeleteFramebuffers = (PFNGLDELETEFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteFramebuffers");
	glGenFramebuffers = (PFNGLGENFRAMEBUFFERSPROC)SDL_GL_GetProcAddress("glGenFramebuffers");
	glBindFramebuffer = (PFNGLBINDFRAMEBUFFERPROC)SDL_GL_GetProcAddress("glBindFramebuffer");
	glFramebufferTexture2D = (PFNGLFRAMEBUFFERTEXTURE2DPROC)SDL_GL_GetProcAddress("glFramebufferTexture2D");
	glGenRenderbuffers = (PFNGLGENRENDERBUFFERSPROC)SDL_GL_GetProcAddress("glGenRenderbuffers");
	glBindRenderbuffer = (PFNGLBINDRENDERBUFFERPROC)SDL_GL_GetProcAddress("glBindRenderbuffer");
	glRenderbufferStorage = (PFNGLRENDERBUFFERSTORAGEPROC)SDL_GL_GetProcAddress("glRenderbufferStorage");
	glFramebufferRenderbuffer = (PFNGLFRAMEBUFFERRENDERBUFFERPROC)SDL_GL_GetProcAddress("glFramebufferRenderbuffer");
	glCheckFramebufferStatus = (PFNGLCHECKFRAMEBUFFERSTATUSPROC)SDL_GL_GetProcAddress("glCheckFramebufferStatus");
	glDeleteRenderbuffers = (PFNGLDELETERENDERBUFFERSPROC)SDL_GL_GetProcAddress("glDeleteRenderbuffers");
	

	return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && 
		glGetShaderInfoLog && glDeleteShader && glAttachShader && glCreateProgram &&
		glLinkProgram && glValidateProgram && glGetProgramiv && glGetProgramInfoLog &&
		glUseProgram && glGetAttribLocation && glGetUniformLocation && glBindVertexArray && 
		glDeleteVertexArrays && glDeleteBuffers && glDeleteProgram && glGenVertexArrays &&
		glGenBuffers && glBindBuffer && glBufferData && glEnableVertexAttribArray &&
		glVertexAttribPointer && glUniform1i && glDisableVertexAttribArray && glUniform2f &&
		glUniformMatrix4fv && glBindAttribLocation && glGetActiveUniform && glGetActiveAttrib &&
		glUniform4f && glBufferSubData && glDeleteFramebuffers && glGenFramebuffers && 
		glBindFramebuffer && glFramebufferTexture2D && glGenRenderbuffers && glBindRenderbuffer && 
		glRenderbufferStorage && glFramebufferRenderbuffer && glCheckFramebufferStatus && glDeleteRenderbuffers;
}

void bindFramebuffer(void)
{
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

GLuint compileShader(const char* source, GLuint shaderType) {
	// Create ID for shader
	GLuint result = glCreateShader(shaderType);
	// Define shader text
	glShaderSource(result, 1, &source, NULL);
	// Compile shader
	glCompileShader(result);

	//Check vertex shader for errors
	GLint shaderCompiled = GL_FALSE;
	glGetShaderiv( result, GL_COMPILE_STATUS, &shaderCompiled );
	if( shaderCompiled != GL_TRUE ) {
		printf( "Error compilation: %d\n",result );
		
		GLint logLength;
		glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
		if (logLength > 0)
		{
			GLchar *log = (GLchar*)malloc(logLength);
			glGetShaderInfoLog(result, logLength, &logLength, log);
			
			printf("Shader compile log: %s\n" ,log );
			free(log);
		}
		glDeleteShader(result);
		result = 0;
	} else {
		printf( "Shader compile ok Id = %d\n", result);
	}
	return result;
}

GLuint compileProgram_default(const char* vtxFile, const char* fragFile) {

	GLuint programId = 0;
	GLuint vtxShaderId, fragShaderId;

	programId = glCreateProgram();

	vtxShaderId = compileShader(vtxFile, GL_VERTEX_SHADER);

	fragShaderId = compileShader(fragFile, GL_FRAGMENT_SHADER);

	if(vtxShaderId && fragShaderId) {
		// Associate shader with program
		glAttachShader(programId, vtxShaderId);
		glAttachShader(programId, fragShaderId);
		glLinkProgram(programId);
		glValidateProgram(programId);

		// Check the status of the compile/link
		GLint logLen;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
		if(logLen > 0) {
			char* log = (char*) malloc(logLen * sizeof(char));
			// Show any errors as appropriate
			glGetProgramInfoLog(programId, logLen, &logLen, log);
			printf( "Prog Info Log:%s\n" , log );
			free(log);
		}	
	}
	
	if(vtxShaderId) {
		glDeleteShader(vtxShaderId);
	}
	if(fragShaderId) {
		glDeleteShader(fragShaderId);
	}
	return programId;
}
#if 0
GLuint compileProgram(const char* vtxFile, const char* fragFile) {
	GLuint programId = 0;
	GLuint vtxShaderId, fragShaderId;

	programId = glCreateProgram();

	FILE *f = fopen(vtxFile, "rb");
	fseek(f, 0, SEEK_END);
	long fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  

	char *string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);
	string[fsize] = 0;

	vtxShaderId = compileShader(string, GL_VERTEX_SHADER);
	free(string);
	
	f = fopen(fragFile, "rb");
	fseek(f, 0, SEEK_END);
	fsize = ftell(f);
	fseek(f, 0, SEEK_SET);  

	string = malloc(fsize + 1);
	fread(string, fsize, 1, f);
	fclose(f);
	string[fsize] = 0;
	
	fragShaderId = compileShader(string, GL_FRAGMENT_SHADER);
	free(string);

	if(vtxShaderId && fragShaderId) {
		// Associate shader with program
		glAttachShader(programId, vtxShaderId);
		glAttachShader(programId, fragShaderId);
		glLinkProgram(programId);
		glValidateProgram(programId);

		// Check the status of the compile/link
		GLint logLen;
		glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
		if(logLen > 0) {
			char* log = (char*) malloc(logLen * sizeof(char));
			// Show any errors as appropriate
			glGetProgramInfoLog(programId, logLen, &logLen, log);
			printf( "Prog Info Log:%s\n" , log );
			free(log);
		}
	}
	if(vtxShaderId) {
		glDeleteShader(vtxShaderId);
	}
	if(fragShaderId) {
		glDeleteShader(fragShaderId);
	}
	return programId;
}
#endif

static void ortho2d(float m[4][4], float left, float right, float bottom, float top) {

    m[0][0] = 1; m[0][1] = 0; m[0][2] = 0; m[0][3] = 0;
    m[1][0] = 0; m[1][1] = 1; m[1][2] = 0; m[1][3] = 0;
    m[2][0] = 0; m[2][1] = 0; m[2][2] = 1; m[2][3] = 0;
    m[3][0] = 0; m[3][1] = 0; m[3][2] = 0; m[3][3] = 1;

    m[0][0] = 2.0f / (right - left);
    m[1][1] = 2.0f / (top - bottom);
    m[2][2] = -1.0f;
    m[3][0] = -(right + left) / (right - left);
    m[3][1] = -(top + bottom) / (top - bottom);
}

void SetFontColorGL(int r,int  g,int  b) {

	glGetIntegerv(GL_CURRENT_PROGRAM,&OldProgramId);
    	glUseProgram(Program_fontId);    	
	glUniform4f(fcol, (float)r/255.0f,(float)g/255.0f,(float)b/255.0f,1.0f);

}
void RestorePrgGL() { 

	glUniform4f(fcol, 1.0f,1.0f,1.0f,1.0f);
	glUseProgram(OldProgramId);
}

void blitFontGL(GLuint backBuffer, int idx, int x, int y,float scale) {

	#define OGL_coord(a,b)  (((float)(a)*2.0f)/(float)(b))
	
    	float dw = (float)GLYPH_WIDTH*scale;	 
 	float dh = (float)GLYPH_HEIGHT*scale;

        float dx=OGL_coord(x,SCREEN_W)-1.0f;
        float dy=1.0f-OGL_coord(y+dh,SCREEN_H);
        float odw= OGL_coord(dw,SCREEN_W);
        float odh= OGL_coord(dh,SCREEN_H);                
  	float rx=dx+odw;
  	float ry=dy+odh;

	float u0=UV[idx][0],u1=UV[idx][1],v0=UV[idx][2],v1=UV[idx][3];

	glBindVertexArray(font_vao);

        float vertices[4][4] = {
            { dx, dy, u0, v1 },            
            { dx, ry, u0, v0 },
            { rx, dy, u1, v1 },
            { rx, ry, u1, v0 },          
        };
        
	glBindTexture(GL_TEXTURE_2D, backBuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*GL_NEAREST*/);
    	
    	if(o_size!=-1)glUniform2f(o_size, SCREEN_W, SCREEN_H);
        if(t_size!=-1)glUniform2f(t_size, TEXC_W, TEXC_H);        
    	glUniform1i(g_tex_uniform, 0);
    	
	glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); 
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
        
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        
        glDisable(GL_BLEND); 
        
        glBindVertexArray(0);
        glBindTexture(GL_TEXTURE_2D, 0);   
}

void drawTextureGL(GLuint backBuffer) {

    	GLint oldProgramId;
    	
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backBuffer);

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR/*GL_NEAREST*/);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR/*GL_NEAREST*/);
    		
	glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgramId);
    	glUseProgram(ProgramId);
	
    	if(o_size!=-1)glUniform2f(o_size, SCREEN_W, SCREEN_H);
        if(t_size!=-1)glUniform2f(t_size, TEXC_W, TEXC_H);
    	glUniform1i(g_tex_uniform, 0);

	glBindVertexArray(vao);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);	
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glUseProgram(oldProgramId);
}


int initGL()
{
	if (!initGLExtensions())return 1;	
	
	program_Id[0] = compileProgram_default(g_vshader_src,g_fshader_stock_src);
	program_Id[1] = compileProgram_default(g_vshader_src,g_fshader_retro_src);
	program_Id[2] = compileProgram_default(g_vshader_src,g_fshader_scanline_src);
	program_Id[3] = compileProgram_default(g_vshader_src,g_fshader_gray_src);
	Program_fontId = compileProgram_default(g_vshader_src,g_fshader_font_src);
	
	ProgramId=program_Id[0];
	g_tex_uniform = glGetUniformLocation(ProgramId, "u_tex");
	u_texture_projection = glGetUniformLocation(ProgramId, "u_projection");			      

	i_pos   = glGetAttribLocation(ProgramId,  "i_pos");
    	i_coord = glGetAttribLocation(ProgramId,  "i_coord");
	//printf("attribute localiton pos:%d coord:%d \n",i_pos,i_coord);
	
	o_size = glGetUniformLocation(ProgramId, "OutputSize");	
	t_size = glGetUniformLocation(ProgramId, "TextureSize");
	
	fcol = glGetUniformLocation(Program_fontId, "Fontcolor");

	//printf("uniform localiton u_tex:%d u_proj:%d o_size:%d t_size:%d fcol:%d\n",g_tex_uniform,
	//		u_texture_projection,o_size,t_size,fcol);
		
	glGenVertexArrays(1, &vao);
     	glGenBuffers(1, &vbo);
     	
	glUseProgram(Program_fontId);	
	
	ortho2d(projection, -1, 1, -1, 1);
		
    	glUniformMatrix4fv(u_texture_projection, 1, GL_FALSE, (GLfloat *)projection);
    	
	glUseProgram(ProgramId);
	
	if (hw.bottom_left_origin){
	    	ortho2d(projection, -1, 1, 1, -1);
	 }
	else
		ortho2d(projection, -1, 1, -1, 1);
		
    	glUniformMatrix4fv(u_texture_projection, 1, GL_FALSE, (GLfloat *)projection);

	glUniform1i(g_tex_uniform, 0);	
	glUniform4f(fcol, 1.0f,1.0f,1.0f,1.0f);
	
	glGenVertexArrays(1, &font_vao);
	glGenBuffers(1, &font_vbo);
	glBindVertexArray(font_vao);
	glBindBuffer(GL_ARRAY_BUFFER, font_vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 4 * 4, NULL, GL_DYNAMIC_DRAW);
	
	glEnableVertexAttribArray(i_pos);
    	glEnableVertexAttribArray(i_coord);
    	glVertexAttribPointer(i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
    	glVertexAttribPointer(i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));	
    	
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0); 
	
	return 1;	
}

void deinitGL() {

	if (rbo)
		glDeleteRenderbuffers(1, &rbo);
	rbo = 0;
	
	if (fbo)
	        glDeleteFramebuffers(1, &fbo);
	fbo = 0;
	
	if (vao)
        	glDeleteVertexArrays(1, &vao);

    	if (vbo)
        	glDeleteBuffers(1, &vbo);
		
    	vao = 0;
    	vbo = 0;
    	
    	if (font_vao)
        	glDeleteVertexArrays(1, &font_vao);

    	if (font_vbo)
        	glDeleteBuffers(1, &font_vbo);
		
    	font_vao = 0;
    	font_vbo = 0;
    
	for(int i=0;i<MAX_SHADERS;i++)
		glDeleteProgram(program_Id[i]);	
		
	glDeleteProgram(Program_fontId);
	
}

void refresh_vertex_data() {
	
	glUniform2f(o_size, SCREEN_W, SCREEN_H);
	float bottom = (float)TEXC_H /(float) TEX_H;
	float right  = (float)TEXC_W / (float)TEX_W;
	
    	float vertex_data[] = {
        	// pos, coord
        	-1.0f, -1.0f, 0.0f,  bottom, // left-bottom
        	-1.0f,  1.0f, 0.0f,  0.0f,   // left-top
        	 1.0f, -1.0f, right,  bottom,// right-bottom
        	 1.0f,  1.0f, right,  0.0f,  // right-top
    	};
    
    	glBindVertexArray(vao);

    	glBindBuffer(GL_ARRAY_BUFFER, vbo);
    	glBufferData(GL_ARRAY_BUFFER, sizeof(vertex_data), vertex_data, GL_STREAM_DRAW);
		
    	glEnableVertexAttribArray(i_pos);
    	glEnableVertexAttribArray(i_coord);
    	glVertexAttribPointer(i_pos, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, 0);
    	glVertexAttribPointer(i_coord, 2, GL_FLOAT, GL_FALSE, sizeof(float)*4, (void*)(2 * sizeof(float)));		

    	glBindVertexArray(0);
    	glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void printUniformAndAttribute(GLuint program){

	// recuperer le nombre total d'uniforms
	GLint n= 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORMS, &n);

	// recuperer la longueur max occuppee par un nom d'uniform
	GLint length_max= 0;
	glGetProgramiv(program, GL_ACTIVE_UNIFORM_MAX_LENGTH, &length_max);
	// allouer une chaine de caractere
	char name[4096];
 	// recuperer les infos de chaque uniform
	for(int index= 0; index < n; index++)
	{
	    GLint glsl_size;
	    GLenum glsl_type;
	    glGetActiveUniform(program, index, length_max, NULL, &glsl_size, &glsl_type, name);
	    
	    // et son identifiant
	    GLint location= glGetUniformLocation(program, name);
	    
	    printf("uniform %d '%s': location %d, type %x, array_size %d\n", index, name, location, glsl_type, glsl_size);
	}	
 
	// recuperer le nombre total d'attributs
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTES, &n);

	// recuperer la longueur max occuppee par un nom d'attribut
	glGetProgramiv(program, GL_ACTIVE_ATTRIBUTE_MAX_LENGTH, &length_max);
 
	// recuperer les infos de chaque attribut
	for(int index= 0; index < n; index++)
	{
	    GLint glsl_size;
	    GLenum glsl_type;
	    glGetActiveAttrib(program, index, length_max, NULL, &glsl_size, &glsl_type, name);
	    
	    // et son identifiant
	    GLint location= glGetAttribLocation(program, name);
	    
	    printf("attribute %d '%s': location %d, type %x, array_size %d\n", index, name, location, glsl_type, glsl_size);
	}
}

void change_shaders(){

	ProgramId=program_Id[current_shader];
	//printf("(%s)\n",shader_name[current_shader]);
	printUniformAndAttribute(ProgramId);

	i_pos   = glGetAttribLocation(ProgramId,  "i_pos");
    	i_coord = glGetAttribLocation(ProgramId,  "i_coord");
	
	o_size = glGetUniformLocation(ProgramId, "OutputSize");	
	t_size = glGetUniformLocation(ProgramId, "TextureSize");
	g_tex_uniform = glGetUniformLocation(ProgramId, "u_tex");
	u_texture_projection = glGetUniformLocation(ProgramId, "u_projection");
	
	glUseProgram(ProgramId);
	glUniform1i(g_tex_uniform, 0);	
	glUniform2f(o_size, SCREEN_W, SCREEN_H);
        glUniform2f(t_size, TEXC_W, TEXC_H);   
        
        if (hw.bottom_left_origin)
	    	ortho2d(projection, -1, 1, 1, -1);
	else     
       		ortho2d(projection, -1, 1, -1, 1);
       		
    	glUniformMatrix4fv(u_texture_projection, 1, GL_FALSE, (GLfloat *)projection);
		  
    	//printf("attribute localiton pos:%d coord:%d \n",i_pos,i_coord);		
	//printf("uniform localiton u_tex:%d u_proj:%d o_size:%d t_size:%d (fcol:%d)\n",g_tex_uniform,
	//		u_texture_projection,o_size,t_size,fcol);
		
	glUseProgram(0);
    
}

void init_framebuffer(int width, int height,GLuint texture)
{
    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture , 0);
 
    printf("hw  %d %d f:%d %d \n",hw.version_major,hw.version_minor,fbo,texture);
   
    if (hw.depth && hw.stencil) {
    printf("hw depth && stencil\n");
    
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);
    } else if (hw.depth) {
        printf("hw depth\n");
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, rbo);
    }

    if (hw.depth || hw.stencil){
    
    	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    		die("ERROR::FRAMEBUFFER:: Framebuffer is not complete!");
    		
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);

}

