////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
////////////////////////////////////////////////////////////////////////////////

// gui
#define _ANT_ENABLE

// GL libraries
#include "glew.hpp"
#include "GL/freeglut.h"

#ifdef _ANT_ENABLE
#	include "AntTweakBar.h"
#endif // _ANT_ENABLE

// Custom libraries
#include "Algebra.hpp"      // Basic algebra library
#include "Transform.hpp"    // Basic transformations
#include "Framework.hpp"    // utility classes/functions

// Standard librabries
#include <iostream>
#include <sstream>
#include <vector>
#include <stdexcept>
#include <cmath>


////////////////////////////////////////////////////////////////////////////////
// Global variables
//
////////////////////////////////////////////////////////////////////////////////

// Constants
const float PI   = 3.14159265;
const float FOVY = PI*0.5f;

enum {
	// buffers
	BUFFER_GRID_VERTICES = 0,
	BUFFER_GRID_INDEXES,
	BUFFER_COUNT,

	// vertex arrays
	VERTEX_ARRAY_GRID = 0,
	VERTEX_ARRAY_COUNT,

	// samplers
	SAMPLER_LINEAR = 0,
	SAMPLER_BILINEAR,
	SAMPLER_TRILINEAR,
	SAMPLER_ANISOTROPICX2,
	SAMPLER_ANISOTROPICX4,
	SAMPLER_ANISOTROPICX8,
	SAMPLER_ANISOTROPICX16,
	SAMPLER_COUNT,

	// textures
	TEXTURE_CHESSBOARD = 0,
	TEXTURE_WOOD,
	TEXTURE_GRASS,
	TEXTURE_LOD,
	TEXTURE_COUNT,

	// programs
	PROGRAM_RENDER = 0,
	PROGRAM_COUNT
};

// OpenGL objects
GLuint *buffers      = NULL;
GLuint *vertexArrays = NULL;
GLuint *textures     = NULL;
GLuint *samplers     = NULL;
GLuint *programs     = NULL;

// Tools
Affine cameraWorld          = Affine::Translation(Vector3(0,1,0));
Projection cameraProjection = Projection::Perspective(FOVY,
                                                      1.0f,
                                                      0.01f,
                                                      4000.0f);

bool mouseLeft  = false;
bool mouseRight = false;

GLfloat deltaTicks  = 0.0f;
GLfloat tileSize    = 100.0f;  // controls the size of the tile
GLfloat scrollSpeed = 1.0f;    // scrolling speed
GLuint gridSize     = 8;       // controls the size (pixels) of the grid triangles 
GLuint gridVertexCount = 0;
GLuint gridIndexCount  = 0;
GLuint activeTexture   = TEXTURE_CHESSBOARD; // displayed texture
GLuint activeSampler   = SAMPLER_TRILINEAR; // texture filtering method

bool wireframe         = false;
bool scrollTexture     = false;

#ifdef _ANT_ENABLE
float speed = 0.0f; // app speed (in ms)
#endif

////////////////////////////////////////////////////////////////////////////////
// Functions
//
////////////////////////////////////////////////////////////////////////////////

void build_grid() {
	// get number of vertices and number of indexes
	GLuint width = glutGet(GLUT_WINDOW_WIDTH);
	GLuint height = glutGet(GLUT_WINDOW_HEIGHT);
	GLuint hVertexCount = width/gridSize + 1;
	GLuint vVertexCount = height/gridSize + 1;
	std::vector<Vector2> vertices;
	std::vector<GLuint>  indexes;

	// allocate memory
	vertices.reserve(hVertexCount * vVertexCount);
	indexes.reserve((hVertexCount-1)*(vVertexCount-1)*2*3);

	// generate vertices
	for(GLuint x=0; x<hVertexCount; ++x)
		for(GLuint y=0; y<vVertexCount; ++y) {
			vertices.push_back(Vector2(
			                       -1.0f+2.0f*float(x)/float(hVertexCount-1),
			                       -1.0f+2.0f*float(y)/float(vVertexCount-1)));
		}

	// generate indexes
	for(GLuint x=0; x<hVertexCount-1; ++x)
		for(GLuint y=0; y<vVertexCount-1; ++y) {
			// upper triangle
			indexes.push_back((x+1u) * vVertexCount + (y+1u));
			indexes.push_back((x+1u) * vVertexCount + y);
			indexes.push_back(x * vVertexCount + y);
			// lower triangle
			indexes.push_back((x+1u) * vVertexCount + (y+1u));
			indexes.push_back(x * vVertexCount + y);
			indexes.push_back(x * vVertexCount + y +1u );
		}

	// upload to GPU
	glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_GRID_VERTICES]);
		glBufferData(GL_ARRAY_BUFFER, 
		             sizeof(Vector2)*vertices.size(),
		             &vertices[0],
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_GRID_INDEXES]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, 
		             sizeof(GLuint)*indexes.size(),
		             &indexes[0],
		             GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// set vertex array
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_GRID]);
		glEnableVertexAttribArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, buffers[BUFFER_GRID_VERTICES]);
		glVertexAttribPointer(0,2,GL_FLOAT,0,0,FW_BUFFER_OFFSET(0));
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, buffers[BUFFER_GRID_INDEXES]);
#ifdef _ANT_ENABLE
	glBindVertexArray(0);
#endif

	// record data count
	gridVertexCount = vertices.size();
	gridIndexCount  = indexes.size();
}


#ifdef _ANT_ENABLE
static void TW_CALL set_camera(void *data) {
#else
void set_camera() {
#endif
	cameraWorld = Affine::Translation(Vector3(0,1,0));
}

void set_tile_size() {
#ifdef _ANT_ENABLE
	glUseProgram(programs[PROGRAM_RENDER]);
#endif // _ANT_ENABLE
	glUniform1f(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                 "uTileSize"),
	            tileSize*0.01f);
#ifdef _ANT_ENABLE
	glUseProgram(0);
#endif // _ANT_ENABLE
}

void set_texture() {
#ifdef _ANT_ENABLE
	glUseProgram(programs[PROGRAM_RENDER]);
#endif // _ANT_ENABLE
	glUniform1i(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                 "sDiffuse"),
	            activeTexture);
#ifdef _ANT_ENABLE
	glUseProgram(0);
#endif // _ANT_ENABLE
}

void set_sampler() {
	glBindSampler(activeTexture,
	              samplers[activeSampler]);
}

void build_texture(const fw::Tga& tga, GLuint textureName) {
	glBindTexture(GL_TEXTURE_2D, textures[textureName]);
	if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_LUMINANCE)
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             GL_RED,
		             tga.Width(),
		             tga.Height(),
		             0,
		             GL_RED,
		             GL_UNSIGNED_BYTE,
		             tga.Pixels());
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_LUMINANCE_ALPHA)
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             GL_RG,
		             tga.Width(),
		             tga.Height(),
		             0,
		             GL_RG,
		             GL_UNSIGNED_BYTE,
		             tga.Pixels());
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_BGR)
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             GL_RGB,
		             tga.Width(),
		             tga.Height(),
		             0,
		             GL_BGR,
		             GL_UNSIGNED_BYTE,
		             tga.Pixels());
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_BGRA)
		glTexImage2D(GL_TEXTURE_2D,
		             0,
		             GL_RGBA,
		             tga.Width(),
		             tga.Height(),
		             0,
		             GL_BGRA,
		             GL_UNSIGNED_BYTE,
		             tga.Pixels());
	glGenerateMipmap(GL_TEXTURE_2D);
}

void build_lod_texture(GLuint textureName) {
	const GLubyte colors[] = {
		255,0,0,255,     // red
		0,255,0,255,     // green
		0,0,255,255,     // blue
		255,0,255,255,   // magenta
		0,255,255,255,   // cyan
		255,255,0,255,   // yellow
	};
	GLubyte *texels = new GLubyte[256*256*4];
	glBindTexture(GL_TEXTURE_2D, textures[textureName]);
	// fill texels and upload to GL
	for(GLint i=0; i<9; ++i) {
		GLint lodsize = 256>>i;
		for(GLint j=0;j<lodsize*lodsize; ++j)
			memcpy(texels+j*4, colors+(i%6)*4, 4);

		glTexImage2D(GL_TEXTURE_2D,
		             i,
		             GL_RGBA,
		             lodsize,
		             lodsize,
		             0,
		             GL_RGBA,
		             GL_UNSIGNED_BYTE,
		             texels);
	}

	// clean up
	delete[] texels;
}

#ifdef _ANT_ENABLE

static void TW_CALL toggle_fullscreen(void *data) {
	// toggle fullscreen
	glutFullScreenToggle();
}

static void TW_CALL set_filtering_mode_cb(const void *value, void *clientData) {
	activeSampler = *(const GLint *)value;
	set_sampler();
}

static void TW_CALL get_filtering_mode_cb(void *value, void *clientData) {
	*(GLint *)value = activeSampler;
}

static void TW_CALL set_texture_cb(const void *value, void *clientData) {
	activeTexture = *(const GLint *)value;
	set_texture();
}

static void TW_CALL get_texture_cb(void *value, void *clientData) {
	*(GLint *)value = activeTexture;
}

static void TW_CALL set_tile_size_cb(const void *value, void *clientData) {
	tileSize = *(const GLfloat *)value;
	set_tile_size();
}

static void TW_CALL get_tile_size_cb(void *value, void *clientData) {
	*(GLfloat *)value = tileSize;
}

#endif

////////////////////////////////////////////////////////////////////////////////
// on init cb
void on_init() {
	// alloc names
	buffers      = new GLuint[BUFFER_COUNT];
	vertexArrays = new GLuint[VERTEX_ARRAY_COUNT];
	textures     = new GLuint[TEXTURE_COUNT];
	samplers     = new GLuint[SAMPLER_COUNT];
	programs     = new GLuint[PROGRAM_COUNT];

	// gen names
	glGenBuffers(BUFFER_COUNT, buffers);
	glGenVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glGenTextures(TEXTURE_COUNT, textures);
	glGenSamplers(SAMPLER_COUNT, samplers);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		programs[i] = glCreateProgram();

	// configure texture
	glActiveTexture(GL_TEXTURE0+TEXTURE_CHESSBOARD);
		build_texture(fw::Tga("chessboard.tga"), TEXTURE_CHESSBOARD);
	glActiveTexture(GL_TEXTURE0+TEXTURE_WOOD);
		build_texture(fw::Tga("wood.tga"), TEXTURE_WOOD);
	glActiveTexture(GL_TEXTURE0+TEXTURE_GRASS);
		build_texture(fw::Tga("grass.tga"), TEXTURE_GRASS);
	glActiveTexture(GL_TEXTURE0+TEXTURE_LOD);
		build_lod_texture(TEXTURE_LOD);

	// configure samplers
	glSamplerParameteri(samplers[SAMPLER_LINEAR],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_LINEAR],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR);

	glSamplerParameteri(samplers[SAMPLER_BILINEAR],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_BILINEAR],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_NEAREST);

	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
//	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
//	                    GL_TEXTURE_WRAP_S,
//	                    GL_CLAMP_TO_EDGE);

	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX2],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX2],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(samplers[SAMPLER_ANISOTROPICX2],
	                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
	                    2.0f);

	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX4],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX4],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(samplers[SAMPLER_ANISOTROPICX4],
	                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
	                    4.0f);

	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX8],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX8],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(samplers[SAMPLER_ANISOTROPICX8],
	                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
	                    8.0f);

	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX16],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_ANISOTROPICX16],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);
	glSamplerParameterf(samplers[SAMPLER_ANISOTROPICX16],
	                    GL_TEXTURE_MAX_ANISOTROPY_EXT,
	                    16.0f);

	// set sampler
	set_sampler();

	// configure programs
	fw::build_glsl_program(programs[PROGRAM_RENDER],
	                       "grid.glsl",
	                       "",
	                       GL_TRUE);
	glUseProgram(programs[PROGRAM_RENDER]);
	set_texture();
	set_tile_size();

//	glCullFace(GL_FRONT);
//	glEnable(GL_CULL_FACE);

#ifdef _ANT_ENABLE
	// start ant
	TwInit(TW_OPENGL, NULL);
	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	// Create a new bar
	TwBar* menuBar = TwNewBar("menu");
	TwDefine("menu size='220 190'");
	TwDefine("menu position='0 0'");
	TwDefine("menu alpha='255'");
	TwDefine("menu valueswidth=85");

	TwAddVarRO(menuBar,
	           "speed (ms)",
	           TW_TYPE_FLOAT,
	           &speed,
	           "");

	TwAddButton( menuBar,
	             "fullscreen",
	             &toggle_fullscreen,
	             NULL,
	             "label='toggle fullscreen'");

	TwAddVarRW(menuBar,
	           "wireframe",
	           TW_TYPE_BOOLCPP,
	           &wireframe,
	           "true='ON' false='OFF'");

	TwAddSeparator( menuBar,
	                "options",
	                NULL );

	TwAddVarCB( menuBar,
	            "tileFactor",
	            TW_TYPE_FLOAT,
	            &set_tile_size_cb,
	            &get_tile_size_cb,
	            NULL,
	            "min=1.0 max=500.0 step=1.0");

	TwEnumVal samplerModeEV[] = {
		{SAMPLER_LINEAR,         "Linear"},
		{SAMPLER_BILINEAR,       "Bilinear" },
		{SAMPLER_TRILINEAR,      "Trilinear"},
		{SAMPLER_ANISOTROPICX2,  "Anisotropic x2"},
		{SAMPLER_ANISOTROPICX4,  "Anisotropic x4"},
		{SAMPLER_ANISOTROPICX8,  "Anisotropic x8"},
		{SAMPLER_ANISOTROPICX16, "Anisotropic x16"}
	};
	TwType filterType= TwDefineEnum("Filter", samplerModeEV, 7);
	TwAddVarCB(menuBar,
	           "filtering",
	           filterType,
	           &set_filtering_mode_cb,
	           &get_filtering_mode_cb,
	           NULL,
	           "help='Change texture filtering method.' ");

	TwEnumVal textureEV[] = {
		{TEXTURE_CHESSBOARD, "Chessboard"},
		{TEXTURE_WOOD,       "Wood" },
		{TEXTURE_GRASS,      "Grass"},
		{TEXTURE_LOD,        "LOD"}
	};
	TwType textureType= TwDefineEnum("Texture", textureEV, 4);
	TwAddVarCB(menuBar,
	           "texture",
	           textureType,
	           &set_texture_cb,
	           &get_texture_cb,
	           NULL,
	           "help='Change tiled texture.' ");

	TwAddVarRW(menuBar,
	           "scrolling",
	           TW_TYPE_BOOLCPP,
	           &scrollTexture,
	           "true='ON' false='OFF'");

	TwAddVarRW(menuBar,
	           "scroll speed",
	           TW_TYPE_FLOAT,
	           &scrollSpeed,
	           "step=0.1 min=-50.0 max=50.0");

	TwAddButton( menuBar,
	             "reset camera",
	             &set_camera,
	             NULL,
	             "");

#endif // _ANT_ENABLE
	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on clean cb
void on_clean() {
	// delete objects
	glDeleteBuffers(BUFFER_COUNT, buffers);
	glDeleteVertexArrays(VERTEX_ARRAY_COUNT, vertexArrays);
	glDeleteTextures(TEXTURE_COUNT, textures);
	glDeleteSamplers(SAMPLER_COUNT, samplers);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		glDeleteProgram(programs[i]);

	// release memory
	delete[] buffers;
	delete[] vertexArrays;
	delete[] textures;
	delete[] samplers;
	delete[] programs;

#ifdef _ANT_ENABLE
	TwTerminate();
#endif // _ANT_ENABLE

	fw::check_gl_error();
}


////////////////////////////////////////////////////////////////////////////////
// on update cb
void on_update() {
	// Variables
	static fw::Timer deltaTimer;
	GLint windowWidth  = glutGet(GLUT_WINDOW_WIDTH);
	GLint windowHeight = glutGet(GLUT_WINDOW_HEIGHT);

	// stop timing and set delta
	deltaTimer.Stop();
	deltaTicks = deltaTimer.Ticks();
#ifdef _ANT_ENABLE
	speed = deltaTicks*1000.0f;
#endif

	// set viewport
	glViewport(0,0,windowWidth, windowHeight);

	// clear back buffer
	glClear(GL_COLOR_BUFFER_BIT);

	// compute camera's world axis and position
	Matrix3x3 camAxis = cameraWorld.GetUnitAxis();
	Vector3 camPos    = cameraWorld.GetPosition();

	// render the grid
#ifdef _ANT_ENABLE
	glUseProgram(programs[PROGRAM_RENDER]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_GRID]);
	glBindSampler(activeTexture,
	              samplers[activeSampler]);
#endif // _ANT_ENABLE

	// scrolling
	static GLfloat scroll = 0.0f;
	if(scrollTexture) {
		scroll = fmod(scroll - 0.25f*deltaTicks*scrollSpeed, 50.0f);
		glUniform1f(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                  "uTextureOffset"),
	                scroll);
	}

	// update transformations
	Matrix4x4 mvp = cameraProjection.ExtractTransformMatrix()
	              * cameraWorld.ExtractInverseTransformMatrix();

	glUniformMatrix4fv(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uModelViewProjection"),
	                   1,
	                   0,
	                   reinterpret_cast<float*>(&mvp));
	glUniformMatrix3fv(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uEyeAxis"),
	                   1,
	                   0,
	                   reinterpret_cast<float*>(&camAxis));
	glUniform3fv(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                  "uEyePos"),
	             1,
	             reinterpret_cast<float*>(&camPos));

	if(wireframe)
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	else
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	// draw
	glDrawElements(GL_TRIANGLES,
	               gridIndexCount,
	               GL_UNSIGNED_INT,
	               FW_BUFFER_OFFSET(0));

	// start ticking
	deltaTimer.Start();

#ifdef _ANT_ENABLE
	// back to default vertex array
	glUseProgram(0);
	glBindVertexArray(0);
	glBindSampler(activeTexture,
	              0);
	TwDraw();

#endif // _ANT_ENABLE

	fw::check_gl_error();

	glutSwapBuffers();
	glutPostRedisplay();
}


////////////////////////////////////////////////////////////////////////////////
// on resize cb
void on_resize(GLint w, GLint h) {
#ifdef _ANT_ENABLE
	TwWindowSize(w, h);
#endif

	// build grid
	build_grid();

	// update projection
#if 0
	cameraProjection.FitWidthToAspect(float(w)/float(h)*1.5f); // debug
#else
	cameraProjection.FitWidthToAspect(float(w)/float(h));
#endif
	// update fov
#ifdef _ANT_ENABLE
	glUseProgram(programs[PROGRAM_RENDER]);
#endif // _ANT_ENABLE
	glUniform2f(glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uTanFov"),
	                   tan(FOVY*0.5f)*float(w)/float(h),
	                   tan(FOVY*0.5f) );
#ifdef _ANT_ENABLE
	glUseProgram(0);
#endif // _ANT_ENABLE
}


////////////////////////////////////////////////////////////////////////////////
// on key down cb
void on_key_down(GLubyte key, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1==TwEventKeyboardGLUT(key, x, y))
		return;
#endif
	if (key==27) // escape
		glutLeaveMainLoop();
	if(key=='r')
		set_camera(NULL);
	if(key=='s')
		scrollTexture = !scrollTexture;
	if(key=='w')
		wireframe = !wireframe;
	if(key=='f')
		glutFullScreenToggle();
	if(key=='p')
		fw::save_gl_front_buffer(0,
		                         0,
		                         glutGet(GLUT_WINDOW_WIDTH),
		                         glutGet(GLUT_WINDOW_HEIGHT));

}


////////////////////////////////////////////////////////////////////////////////
// on mouse button cb
void on_mouse_button(GLint button, GLint state, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseButtonGLUT(button, state, x, y))
		return;
#endif // _ANT_ENABLE
	if(state==GLUT_DOWN) {
		mouseLeft  |= button == GLUT_LEFT_BUTTON;
		mouseRight |= button == GLUT_RIGHT_BUTTON;
	}
	else {
		mouseLeft  &= button == GLUT_LEFT_BUTTON ? false : mouseLeft;
		mouseRight  &= button == GLUT_RIGHT_BUTTON ? false : mouseRight;
	}
	if(button == 3)
		cameraWorld.TranslateLocal(Vector3(0,0,-0.15f));
	if(button == 4)
		cameraWorld.TranslateLocal(Vector3(0,0,0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// on mouse motion cb
void on_mouse_motion(GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwEventMouseMotionGLUT(x,y))
		return;
#endif // _ANT_ENABLE

	static GLint sMousePreviousX = 0;
	static GLint sMousePreviousY = 0;
	const GLint MOUSE_XREL = x-sMousePreviousX;
	const GLint MOUSE_YREL = y-sMousePreviousY;
	sMousePreviousX = x;
	sMousePreviousY = y;

	if(mouseLeft) {
		cameraWorld.RotateAboutLocalX(-2.0f*MOUSE_YREL*deltaTicks);
		cameraWorld.RotateAboutWorldY(-2.0f*MOUSE_XREL*deltaTicks);
	}
	if(mouseRight) {
		cameraWorld.TranslateLocal(deltaTicks*Vector3(-2.0f*MOUSE_XREL,
		                                               2.0f*MOUSE_YREL,
		                                               0));
	}
}


////////////////////////////////////////////////////////////////////////////////
// on mouse wheel cb
void on_mouse_wheel(GLint wheel, GLint direction, GLint x, GLint y) {
#ifdef _ANT_ENABLE
	if(1 == TwMouseWheel(wheel))
		return;
#endif // _ANT_ENABLE
	cameraWorld.TranslateLocal(Vector3(0,0,float(direction)*0.15f));
}


////////////////////////////////////////////////////////////////////////////////
// Main
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	const GLuint CONTEXT_MAJOR = 3;
	const GLuint CONTEXT_MINOR = 3;

	// init glut
	glutInit(&argc, argv);
	glutInitContextVersion(CONTEXT_MAJOR ,CONTEXT_MINOR);
#ifdef _ANT_ENABLE
	glutInitContextFlags(GLUT_DEBUG);
	glutInitContextProfile(GLUT_COMPATIBILITY_PROFILE);
#else
	glutInitContextFlags(GLUT_DEBUG | GLUT_FORWARD_COMPATIBLE);
	glutInitContextProfile(GLUT_CORE_PROFILE);
#endif

	// build window
	glutInitDisplayMode(/*GLUT_DEPTH |*/ GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(800, 600);
	glutInitWindowPosition(0, 0);
	glutCreateWindow("texture filtering");

	// init glew
	glewExperimental = GL_TRUE; // segfault on GenVertexArrays on Nvidia otherwise
	GLenum err = glewInit();
	if(GLEW_OK != err)
	{
		std::stringstream ss;
		ss << err;
		std::cerr << "glewInit() gave error " << ss.str() << std::endl;
		return 1;
	}

	// glewInit generates an INVALID_ENUM error for some reason...
	glGetError();

	// set callbacks
	glutCloseFunc(&on_clean);
	glutReshapeFunc(&on_resize);
	glutDisplayFunc(&on_update);
	glutKeyboardFunc(&on_key_down);
	glutMouseFunc(&on_mouse_button);
	glutPassiveMotionFunc(&on_mouse_motion);
	glutMotionFunc(&on_mouse_motion);
	glutMouseWheelFunc(&on_mouse_wheel);

	// run
	try
	{
		// run demo
		on_init();
		glutMainLoop();
	}
	catch(std::exception& e)
	{
		std::cerr << "Fatal exception: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}

