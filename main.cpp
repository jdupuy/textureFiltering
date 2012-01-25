////////////////////////////////////////////////////////////////////////////////
// \author   Jonathan Dupuy
//
////////////////////////////////////////////////////////////////////////////////

// enable gui
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
const float FOVY = PI*0.25f;

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
	TEXTURE_GRASS,
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
Affine invCameraWorld       = Affine::Translation(Vector3(0,-2,-10));
Projection cameraProjection = Projection::Perspective(FOVY,
                                                      1.0f,
                                                      0.2f,
                                                      10000.0f);

bool mouseLeft  = false;
bool mouseRight = false;

float tileSize = 1.0f;
GLuint gridSize = 4;
GLuint gridVertexCount = 0;
GLuint gridIndexCount  = 0;
GLuint activeTexture   = TEXTURE_GRASS;
GLuint activeSampler   = SAMPLER_ANISOTROPICX16;

#ifdef _ANT_ENABLE

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
	glBindVertexArray(0);

	// record data count
	gridVertexCount = vertices.size();
	gridIndexCount  = indexes.size();
}


void set_texture() {
	glProgramUniform1i(programs[PROGRAM_RENDER],
	                   glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "sDiffuse"),
	                   activeTexture);
}

void set_sampler() {
	glBindSampler(activeTexture,
	              samplers[activeSampler]);
}

void build_texture(const fw::Tga& tga, GLuint textureName) {
	glBindTexture(GL_TEXTURE_2D, textures[textureName]);
	if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_LUMINANCE)
		glTexImage2D( GL_TEXTURE_2D,
		              0,
		              GL_RED,
		              tga.Width(),
		              tga.Height(),
		              0,
		              GL_RED,
		              GL_UNSIGNED_BYTE,
		              tga.Pixels() );
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_LUMINANCE_ALPHA)
		glTexImage2D( GL_TEXTURE_2D,
		              0,
		              GL_RG,
		              tga.Width(),
		              tga.Height(),
		              0,
		              GL_RG,
		              GL_UNSIGNED_BYTE,
		              tga.Pixels() );
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_BGR)
		glTexImage2D( GL_TEXTURE_2D,
		              0,
		              GL_RGB,
		              tga.Width(),
		              tga.Height(),
		              0,
		              GL_BGR,
		              GL_UNSIGNED_BYTE,
		              tga.Pixels() );
	else if(tga.PixelFormat() == fw::Tga::PIXEL_FORMAT_BGRA)
		glTexImage2D( GL_TEXTURE_2D,
		              0,
		              GL_RGBA,
		              tga.Width(),
		              tga.Height(),
		              0,
		              GL_BGRA,
		              GL_UNSIGNED_BYTE,
		              tga.Pixels() );
	glGenerateMipmap(GL_TEXTURE_2D);
}

#ifdef _ANT_ENABLE

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
//	glGenSamplers(SAMPLER_COUNT, samplers);
	for(GLuint i=0; i<PROGRAM_COUNT;++i)
		programs[i] = glCreateProgram();

	// configure texture
	glActiveTexture(GL_TEXTURE0+TEXTURE_CHESSBOARD);
		build_texture(fw::Tga("chessboard.tga"), TEXTURE_CHESSBOARD);
	glActiveTexture(GL_TEXTURE0+TEXTURE_GRASS);
		build_texture(fw::Tga("grass.tga"), TEXTURE_GRASS);

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
	                    GL_NEAREST_MIPMAP_LINEAR);

	glSamplerParameteri(samplers[SAMPLER_TRILINEAR],
	                    GL_TEXTURE_MAG_FILTER,
	                    GL_LINEAR);
	glSamplerParameteri(samplers[SAMPLER_BILINEAR],
	                    GL_TEXTURE_MIN_FILTER,
	                    GL_LINEAR_MIPMAP_LINEAR);

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

	// configure buffer objects
	build_grid();

	// configure programs
	fw::build_glsl_program(programs[PROGRAM_RENDER],
	                       "grid.glsl",
	                       "",
	                       GL_TRUE);
	set_texture();

	glEnable(GL_DEPTH_TEST);
//	glEnable(GL_CULL_FACE);
	glClearColor(0.13,0.13,0.15,1.0);

#ifdef _ANT_ENABLE
	// start ant
	TwInit(TW_OPENGL, NULL);
	// send the ''glutGetModifers'' function pointer to AntTweakBar
	TwGLUTModifiersFunc(glutGetModifiers);

	// Create a new bar
	TwBar* menuBar = TwNewBar("menu");
	TwDefine("menu size='250 150'");
//	TwAddButton( menuBar,
//	             "fullscreen",
//	             &toggle_fullscreen,
//	             NULL,
//	             "label='toggle fullscreen'");
	TwAddVarRO( menuBar,
	            "tileSize",
	            TW_TYPE_FLOAT,
	            &tileSize,
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
//	glDeleteSamplers(SAMPLER_COUNT, samplers);
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
	// Global variable
	GLint windowWidth  = glutGet(GLUT_WINDOW_WIDTH);
	GLint windowHeight = glutGet(GLUT_WINDOW_HEIGHT);
	float aspect       = float(windowWidth)/float(windowHeight);

#ifdef _ANT_ENABLE

#endif // _ANT_ENABLE

	// set viewport
	glViewport(0,0,windowWidth, windowHeight);

	// clear back buffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// update transformations
	Matrix4x4 mvp = cameraProjection.ExtractTransformMatrix()
	              * invCameraWorld.ExtractTransformMatrix();

	// compute camera's world axis and position
	Matrix3x3 camAxis = invCameraWorld.GetUnitAxis().Transpose();
	Matrix4x4 tmp     = invCameraWorld.ExtractInverseTransformMatrix();
	Vector3 camPos    = Vector3(tmp[3][0], tmp[3][1], tmp[3][2]);
	cameraProjection = Projection::Perspective(FOVY,
	                                           aspect,
	                                           (abs(camPos[1])+0.01f) * 0.2f,
	                                           (abs(camPos[1])+0.01f) * 10000.0f);

	glProgramUniformMatrix4fv(programs[PROGRAM_RENDER],
	                          glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uModelViewProjection"),
	                          1,
	                          0,
	                          reinterpret_cast<float*>(&mvp));
	glProgramUniformMatrix3fv(programs[PROGRAM_RENDER],
	                          glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uCameraWorldAxis"),
	                          1,
	                          0,
	                          reinterpret_cast<float*>(&camAxis));
	glProgramUniform3fv(programs[PROGRAM_RENDER],
	                    glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uCameraWorldPosition"),
	                    1,
	                    reinterpret_cast<float*>(&camPos));

//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	// render the model
	glUseProgram(programs[PROGRAM_RENDER]);
	glBindVertexArray(vertexArrays[VERTEX_ARRAY_GRID]);
	glDrawElements(GL_TRIANGLES,
	               gridIndexCount,
	               GL_UNSIGNED_INT,
	               FW_BUFFER_OFFSET(0));

//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

//	glDrawArrays( GL_POINTS,
//	              0,
//	              gridVertexCount);


#ifdef _ANT_ENABLE
	// back to default vertex array
	glUseProgram(0);
	glBindVertexArray(0);
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
	// update projection
	cameraProjection.FitWidthToAspect(float(w)/float(h));

	// update fov
	glProgramUniform2f(programs[PROGRAM_RENDER],
	                   glGetUniformLocation(programs[PROGRAM_RENDER],
	                                         "uTanFov"),
	                   tan(FOVY)*float(w)/float(h),
	                   tan(FOVY) );
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
	if(state==GLUT_DOWN)
	{
		mouseLeft  |= button == GLUT_LEFT_BUTTON;
		mouseRight |= button == GLUT_RIGHT_BUTTON;
	}
	else
	{
		mouseLeft  &= button == GLUT_LEFT_BUTTON ? false : mouseLeft;
		mouseRight  &= button == GLUT_RIGHT_BUTTON ? false : mouseRight;
	}
	if(button == 3)
		invCameraWorld.TranslateWorld(Vector3(0,0,0.15f));
	if(button == 4)
		invCameraWorld.TranslateWorld(Vector3(0,0,-0.15f));
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

	if(mouseLeft)
	{
		invCameraWorld.RotateAboutWorldX(-0.01f*MOUSE_YREL);
		invCameraWorld.RotateAboutLocalY( 0.01f*MOUSE_XREL);
	}
	if(mouseRight)
	{
		invCameraWorld.TranslateWorld(Vector3( 0.01f*MOUSE_XREL,
		                                      -0.01f*MOUSE_YREL,
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
}


////////////////////////////////////////////////////////////////////////////////
// Main
//
////////////////////////////////////////////////////////////////////////////////
int main(int argc, char** argv) {
	const GLuint CONTEXT_MAJOR = 4;
	const GLuint CONTEXT_MINOR = 2;

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
	glutInitDisplayMode(GLUT_DEPTH | GLUT_DOUBLE | GLUT_RGBA);
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

