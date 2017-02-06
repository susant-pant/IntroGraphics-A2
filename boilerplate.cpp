// ==========================================================================
// Barebones OpenGL Core Profile Boilerplate
//    using the GLFW windowing system (http://www.glfw.org)
//
// Loosely based on
//  - Chris Wellons' example (https://github.com/skeeto/opengl-demo) and
//  - Camilla Berglund's example (http://www.glfw.org/docs/latest/quick.html)
//
// Author:  Sonny Chan, University of Calgary
// Date:    December 2015
// ==========================================================================

#include <iostream>
#include <fstream>
#include <algorithm>
#include <string>
#include <vector>
#include "glm/glm.hpp"
#include <iterator>

// specify that we want the OpenGL core profile before including GLFW headers
#define GLFW_INCLUDE_GLCOREARB
#define GL_GLEXT_PROTOTYPES
#include <GLFW/glfw3.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace std;
using namespace glm;

const char* image_name = " ";
float zoom = 1.f;
float rotat = 0.f;
bool space = false;
float new_x = 0.f;
float new_y = 0.f;
float mouse_startX = 0.f;
float mouse_startY = 0.f;
float mouse_endX = 0.f;
float mouse_endY = 0.f;
bool drag = false;

// --------------------------------------------------------------------------
// OpenGL utility and support function prototypes

void QueryGLVersion();
bool CheckGLErrors();

string LoadSource(const string &filename);
GLuint CompileShader(GLenum shaderType, const string &source);
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader);

// --------------------------------------------------------------------------
// Functions to set up OpenGL shader programs for rendering

struct MyShader
{
	// OpenGL names for vertex and fragment shaders, shader program
	GLuint  vertex;
	GLuint  fragment;
	GLuint  program;

	// initialize shader and program names to zero (OpenGL reserved value)
	MyShader() : vertex(0), fragment(0), program(0)
	{}
};

// load, compile, and link shaders, returning true if successful
bool InitializeShaders(MyShader *shader)
{
	// load shader source from files
	string vertexSource = LoadSource("vertex.glsl");
	string fragmentSource = LoadSource("fragment.glsl");
	if (vertexSource.empty() || fragmentSource.empty()) return false;

	// compile shader source into shader objects
	shader->vertex = CompileShader(GL_VERTEX_SHADER, vertexSource);
	shader->fragment = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

	// link shader program
	shader->program = LinkProgram(shader->vertex, shader->fragment);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate shader-related objects
void DestroyShaders(MyShader *shader)
{
	// unbind any shader programs and destroy shader objects
	glUseProgram(0);
	glDeleteProgram(shader->program);
	glDeleteShader(shader->vertex);
	glDeleteShader(shader->fragment);
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing textures

struct MyTexture
{
	GLuint textureID;
	GLuint target;
	int width;
	int height;

	// initialize object names to zero (OpenGL reserved value)
	MyTexture() : textureID(0), target(0), width(0), height(0)
	{}
};

bool InitializeTexture(MyTexture* texture, const char* filename, GLuint target = GL_TEXTURE_2D)
{
	int numComponents;
	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename, &texture->width, &texture->height, &numComponents, 0);
	if (data != nullptr)
	{
		texture->target = target;
		glGenTextures(1, &texture->textureID);
		glBindTexture(texture->target, texture->textureID);
		GLuint format = numComponents == 3 ? GL_RGB : GL_RGBA;
		glTexImage2D(texture->target, 0, format, texture->width, texture->height, 0, format, GL_UNSIGNED_BYTE, data);

		// Note: Only wrapping modes supported for GL_TEXTURE_RECTANGLE when defining
		// GL_TEXTURE_WRAP are GL_CLAMP_TO_EDGE or GL_CLAMP_TO_BORDER
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(texture->target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(texture->target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		// Clean up
		glBindTexture(texture->target, 0);
		stbi_image_free(data);
		return !CheckGLErrors();
	}
	return true; //error
}

// deallocate texture-related objects
void DestroyTexture(MyTexture *texture)
{
	glBindTexture(texture->target, 0);
	glDeleteTextures(1, &texture->textureID);
}

void SaveImage(const char* filename, int width, int height, unsigned char *data, int numComponents = 3, int stride = 0)
{
	if (!stbi_write_png(filename, width, height, numComponents, data, stride))
		cout << "Unable to save image: " << filename << endl;
}

// --------------------------------------------------------------------------
// Functions to set up OpenGL buffers for storing geometry data

struct MyGeometry
{
	// OpenGL names for array buffer objects, vertex array object
	GLuint  vertexBuffer;
	GLuint  textureBuffer;
	GLuint  colourBuffer;
	GLuint  vertexArray;
	GLsizei elementCount;

	// initialize object names to zero (OpenGL reserved value)
	MyGeometry() : vertexBuffer(0), colourBuffer(0), vertexArray(0), elementCount(0)
	{}
};

// create buffers and fill with geometry data, returning true if successful
bool InitializeGeometry(MyGeometry *geometry, float height, float width)
{
	float x = 1.f;
	float y = 1.f;

	if (height > width)
		x = (width / height);

	else if(width > height)
		y = (height / width);
	
	// three vertex positions and assocated colours of a triangle
	const GLfloat vertices[][2] = {

		{-x, -y},
		{-x, y},
		{x, -y},
		
		{-x, y},
		{x, -y},
		{x, y}
	};

	const GLfloat textureCoords[][2] = {
		{0.f, 0.f},
		{0.f, height},
		{width, 0.f},
		
		{0.f, height},
		{width, 0.f},
		{width,height}
	};

	const GLfloat colours[][3] = {};
	geometry->elementCount = 6;

	// these vertex attribute indices correspond to those specified for the
	// input variables in the vertex shader
	const GLuint VERTEX_INDEX = 0;
	const GLuint COLOUR_INDEX = 1;
	const GLuint TEXTURE_INDEX = 2;

	// create an array buffer object for storing our vertices
	glGenBuffers(1, &geometry->vertexBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	//
	glGenBuffers(1, &geometry->textureBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(textureCoords), textureCoords, GL_STATIC_DRAW);

	// create another one for storing our colours
	glGenBuffers(1, &geometry->colourBuffer);
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(colours), colours, GL_STATIC_DRAW);

	// create a vertex array object encapsulating all our vertex attributes
	glGenVertexArrays(1, &geometry->vertexArray);
	glBindVertexArray(geometry->vertexArray);

	// associate the position array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->vertexBuffer);
	glVertexAttribPointer(VERTEX_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(VERTEX_INDEX);

	// Tell openGL how the data is formatted
	glBindBuffer(GL_ARRAY_BUFFER, geometry->textureBuffer);
	glVertexAttribPointer(TEXTURE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(TEXTURE_INDEX);

	// assocaite the colour array with the vertex array object
	glBindBuffer(GL_ARRAY_BUFFER, geometry->colourBuffer);
	glVertexAttribPointer(COLOUR_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(COLOUR_INDEX);

	// unbind our buffers, resetting to default state
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

	// check for OpenGL errors and return false if error occurred
	return !CheckGLErrors();
}

// deallocate geometry-related objects
void DestroyGeometry(MyGeometry *geometry)
{
	// unbind and destroy our vertex array object and associated buffers
	glBindVertexArray(0);
	glDeleteVertexArrays(1, &geometry->vertexArray);
	glDeleteBuffers(1, &geometry->vertexBuffer);
	glDeleteBuffers(1, &geometry->colourBuffer);
}

// --------------------------------------------------------------------------
// Rendering function that draws our scene to the frame buffer

void RenderScene(MyGeometry *geometry, MyTexture* texture, MyShader *shader)
{
	// clear screen to a dark grey colour
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// bind our shader program and the vertex array object containing our
	// scene geometry, then tell OpenGL to draw our geometry
	glUseProgram(shader->program);
	glBindVertexArray(geometry->vertexArray);
	glBindTexture(texture->target, texture->textureID);
	glDrawArrays(GL_TRIANGLES, 0, geometry->elementCount);

	// reset state to default (no shader or geometry bound)
	glBindTexture(texture->target, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	// check for an report any OpenGL errors
	CheckGLErrors();
}

// --------------------------------------------------------------------------
// GLFW callback functions

MyShader shader;
MyGeometry geometry;
MyTexture texture;

float redFilter = 0.f;
float blueFilter = 0.f;
float greenFilter = 0.f;
bool red = false;
bool blue = false;
bool green = false;
bool hue = false;

// reports GLFW errors
void ErrorCallback(int error, const char* description)
{
	cout << "GLFW ERROR " << error << ":" << endl;
	cout << description << endl;
}

void reInit(){
	if(!InitializeTexture(&texture, image_name, GL_TEXTURE_RECTANGLE))
		cout << "Program failed to intialize texture!" << endl;
	if (!InitializeGeometry(&geometry, texture.height, texture.width))
		cout << "Program failed to intialize geometry!" << endl;
}

void changeGreyScale(int dora) {
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "greyScale");
	if (loc != -1)
		glUniform1i(loc, dora);
}

void changeFilterType(int wryeah) {
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "filterType");
	if (loc != -1)
		glUniform1i(loc, wryeah);
}

void changeBlurType(int shizaa) {
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "blurType");
	if (loc != -1)
		glUniform1i(loc, shizaa);
}

// handles keyboard input events
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (action == GLFW_PRESS) {
		if (key == GLFW_KEY_ESCAPE)
			glfwSetWindowShouldClose(window, GL_TRUE);
		else if (key == GLFW_KEY_1){
			image_name = "test.jpg";
			reInit();
		}
		else if (key == GLFW_KEY_2){
			image_name = "mandrill.png";
			reInit();
		}
		else if (key == GLFW_KEY_3){
			image_name = "uclogo.png";
			reInit();
		}
		else if (key == GLFW_KEY_4){
			image_name = "aerial.jpg";
			reInit();
		}
		else if (key == GLFW_KEY_5){
			image_name = "thirsk.jpg";
			reInit();
		}
		else if (key == GLFW_KEY_6){
			image_name = "pattern.png";
			reInit();
		}
		else if (key == GLFW_KEY_Q){
			changeGreyScale(0);
			changeFilterType(0);
			changeBlurType(0);
			
			redFilter = 0.f;
			blueFilter = 0.f;
			greenFilter = 0.f;
			glUseProgram(shader.program);
			GLint locr = glGetUniformLocation(shader.program, "redFilter");
			if (locr != -1)
				glUniform1f(locr, redFilter);
			GLint locb = glGetUniformLocation(shader.program, "blueFilter");
			if (locb != -1)
				glUniform1f(locb, blueFilter);
			GLint locg = glGetUniformLocation(shader.program, "greenFilter");
			if (locg != -1)
				glUniform1f(locg, greenFilter);
		}
		else if (key == GLFW_KEY_W){
			changeGreyScale(1);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_E){
			changeGreyScale(2);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_R){
			changeGreyScale(3);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_T){
			changeGreyScale(4);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_Y){
			changeGreyScale(5);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_U){
			changeGreyScale(6);
			changeFilterType(0);
			changeBlurType(0);
		}
		else if (key == GLFW_KEY_Z){
			changeFilterType(1);
		}
		else if (key == GLFW_KEY_X){
			changeFilterType(2);
		}
		else if (key == GLFW_KEY_C){
			changeFilterType(3);
		}
		else if (key == GLFW_KEY_V){
			changeFilterType(0);
			changeBlurType(1);
		}
		else if (key == GLFW_KEY_B){
			changeFilterType(0);
			changeBlurType(2);
		}
		else if (key == GLFW_KEY_N){
			changeFilterType(0);
			changeBlurType(3);
		}
		else if (key == GLFW_KEY_UP){
			if (red){
				if (redFilter < 1.f)
					redFilter += 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "redFilter");
				if (loc != -1)
					glUniform1f(loc, redFilter);
			}
			if (blue){
				if (blueFilter < 1.f)
					blueFilter += 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "blueFilter");
				if (loc != -1)
					glUniform1f(loc, blueFilter);
			}
			if (green){
				if (greenFilter < 1.f)
					greenFilter += 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "greenFilter");
				if (loc != -1)
					glUniform1f(loc, greenFilter);
			}
		}
		else if (key == GLFW_KEY_DOWN){
			if (red){
				if (redFilter > -1.f)
					redFilter -= 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "redFilter");
				if (loc != -1)
					glUniform1f(loc, redFilter);
			}
			if (blue){
				if (blueFilter > -1.f)
					blueFilter -= 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "blueFilter");
				if (loc != -1)
					glUniform1f(loc, blueFilter);
			}
			if (green){
				if (greenFilter > -1.f)
					greenFilter -= 0.05f;
				glUseProgram(shader.program);
				GLint loc = glGetUniformLocation(shader.program, "greenFilter");
				if (loc != -1)
					glUniform1f(loc, greenFilter);
			}
		}
	}
	if (key == GLFW_KEY_SPACE){
		if (action == GLFW_PRESS)
			space = true;
		else if (action == GLFW_RELEASE)
			space = false;
	}

	if (key == GLFW_KEY_F){
		if (action == GLFW_PRESS){
			hue = true;
			changeFilterType(0);
			red = true;
		}
		else if (action == GLFW_RELEASE)
			red = false;
	}
	if (key == GLFW_KEY_G){
		if (action == GLFW_PRESS){
			hue = true;
			changeFilterType(0);
			green = true;
		}
		else if (action == GLFW_RELEASE)
			green = false;
	}
	if (key == GLFW_KEY_H){
		if (action == GLFW_PRESS){
			hue = true;
			changeFilterType(0);
			blue = true;
		}
		else if (action == GLFW_RELEASE)
			blue = false;
	}
	glUseProgram(shader.program);
	GLint loc = glGetUniformLocation(shader.program, "hue");
	if (loc != -1)
		glUniform1i(loc, hue);
}
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
	if (!space){
		if (yoffset < 0){
			zoom *= 0.9;
			glUseProgram(shader.program);
			GLint loc = glGetUniformLocation(shader.program, "zoomVer");
			if (loc != -1)
				glUniform1f(loc, zoom);
			reInit();

		}
		else {
			zoom *= 1.15;
			glUseProgram(shader.program);
			GLint loc = glGetUniformLocation(shader.program, "zoomVer");
			if (loc != -1)
				glUniform1f(loc, zoom);
			reInit();
		}
	}
	else{
		if (yoffset < 0){
			rotat--;
			glUseProgram(shader.program);
			GLint loc = glGetUniformLocation(shader.program, "theta");
			if (loc != -1)
				glUniform1f(loc, (M_PI / 90.f) * rotat);
		}
		else {
			rotat++;
			glUseProgram(shader.program);
			GLint loc = glGetUniformLocation(shader.program, "theta");
			if (loc != -1)
				glUniform1f(loc, (M_PI / 90.f) * rotat);
		}
	}
}

float oriX = 0.f;
float oriY = 0.f;
float r_oriX = 0.f;
float r_oriY = 0.f;

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT) {
		if (action == GLFW_PRESS) {
			mouse_startX = new_x;
			mouse_startY = new_y;
			drag = true;
		}
		else if (action == GLFW_RELEASE) {
			oriX += (r_oriX);
			oriY += (r_oriY);
			drag = false;
		}
	}
}

static void cursor_pos_callback(GLFWwindow* window, double xpos, double ypos)
{
	new_x = ((float)xpos) / 1025*2;
	new_y = ((float)ypos) / 1025*2;
	if(drag){
		mouse_endX = new_x;
		mouse_endY = new_y;
		
		float muda = (mouse_endX - mouse_startX) / zoom;
		float ora = (mouse_startY - mouse_endY) / zoom;
	
		r_oriX = muda*cos(M_PI / 90.f * rotat) - ora*sin(M_PI / 90.f * rotat);
		r_oriY = ora*cos(M_PI / 90.f * rotat) + muda*sin(M_PI / 90.f * rotat);

		glUseProgram(shader.program);
		GLint locX = glGetUniformLocation(shader.program, "displaceX");
		if (locX != -1)
			glUniform1f(locX, (r_oriX + oriX));

		GLint locY = glGetUniformLocation(shader.program, "displaceY");
		if (locY != -1)
			glUniform1f(locY, (r_oriY + oriY));
	}
}


// ==========================================================================
// PROGRAM ENTRY POINT

int main(int argc, char *argv[])
{
	// initialize the GLFW windowing system
	if (!glfwInit()) {
		cout << "ERROR: GLFW failed to initialize, TERMINATING" << endl;
		return -1;
	}
	glfwSetErrorCallback(ErrorCallback);

	// attempt to create a window with an OpenGL 4.1 core profile context
	GLFWwindow *window = 0;
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	window = glfwCreateWindow(1025, 1025, "CPSC 453 OpenGL Boilerplate", 0, 0);
	if (!window) {
		cout << "Program failed to create GLFW window, TERMINATING" << endl;
		glfwTerminate();
		return -1;
	}

	// set keyboard callback function and make our context current (active)
	glfwSetKeyCallback(window, KeyCallback);
	glfwSetScrollCallback(window, scroll_callback);
	glfwSetCursorPosCallback(window, cursor_pos_callback);
	glfwSetMouseButtonCallback(window, mouse_button_callback);
	glfwMakeContextCurrent(window);

	// query and print out information about our OpenGL environment
	QueryGLVersion();

	// call function to load and compile shader programs
	if (!InitializeShaders(&shader)) {
		cout << "Program could not initialize shaders, TERMINATING" << endl;
		return -1;
	}

	if(!InitializeTexture(&texture, "test.jpg", GL_TEXTURE_RECTANGLE))
		cout << "Program failed to intialize texture!" << endl;
		
	// call function to create and fill buffers with geometry data
	if (!InitializeGeometry(&geometry, texture.height, texture.width))
		cout << "Program failed to intialize geometry!" << endl;

	// run an event-triggered main loop
	while (!glfwWindowShouldClose(window))
	{
		// call function to draw our scene
		RenderScene(&geometry, &texture, &shader); //render scene with texture

		glfwSwapBuffers(window);

		glfwPollEvents();
	}

	// clean up allocated resources before exit
	DestroyGeometry(&geometry);
	DestroyShaders(&shader);
	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}

// ==========================================================================
// SUPPORT FUNCTION DEFINITIONS

// --------------------------------------------------------------------------
// OpenGL utility functions

void QueryGLVersion()
{
	// query opengl version and renderer information
	string version = reinterpret_cast<const char *>(glGetString(GL_VERSION));
	string glslver = reinterpret_cast<const char *>(glGetString(GL_SHADING_LANGUAGE_VERSION));
	string renderer = reinterpret_cast<const char *>(glGetString(GL_RENDERER));

	cout << "OpenGL [ " << version << " ] "
		<< "with GLSL [ " << glslver << " ] "
		<< "on renderer [ " << renderer << " ]" << endl;
}

bool CheckGLErrors()
{
	bool error = false;
	for (GLenum flag = glGetError(); flag != GL_NO_ERROR; flag = glGetError())
	{
		cout << "OpenGL ERROR:  ";
		switch (flag) {
		case GL_INVALID_ENUM:
			cout << "GL_INVALID_ENUM" << endl; break;
		case GL_INVALID_VALUE:
			cout << "GL_INVALID_VALUE" << endl; break;
		case GL_INVALID_OPERATION:
			cout << "GL_INVALID_OPERATION" << endl; break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			cout << "GL_INVALID_FRAMEBUFFER_OPERATION" << endl; break;
		case GL_OUT_OF_MEMORY:
			cout << "GL_OUT_OF_MEMORY" << endl; break;
		default:
			cout << "[unknown error code]" << endl;
		}
		error = true;
	}
	return error;
}

// --------------------------------------------------------------------------
// OpenGL shader support functions

// reads a text file with the given name into a string
string LoadSource(const string &filename)
{
	string source;

	ifstream input(filename.c_str());
	if (input) {
		copy(istreambuf_iterator<char>(input),
			istreambuf_iterator<char>(),
			back_inserter(source));
		input.close();
	}
	else {
		cout << "ERROR: Could not load shader source from file " << filename << endl;
	}

	return source;
}

// creates and returns a shader object compiled from the given source
GLuint CompileShader(GLenum shaderType, const string &source)
{
	// allocate shader object name
	GLuint shaderObject = glCreateShader(shaderType);

	// try compiling the source as a shader of the given type
	const GLchar *source_ptr = source.c_str();
	glShaderSource(shaderObject, 1, &source_ptr, 0);
	glCompileShader(shaderObject);

	// retrieve compile status
	GLint status;
	glGetShaderiv(shaderObject, GL_COMPILE_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetShaderiv(shaderObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetShaderInfoLog(shaderObject, info.length(), &length, &info[0]);
		cout << "ERROR compiling shader:" << endl << endl;
		cout << source << endl;
		cout << info << endl;
	}

	return shaderObject;
}

// creates and returns a program object linked from vertex and fragment shaders
GLuint LinkProgram(GLuint vertexShader, GLuint fragmentShader)
{
	// allocate program object name
	GLuint programObject = glCreateProgram();

	// attach provided shader objects to this program
	if (vertexShader)   glAttachShader(programObject, vertexShader);
	if (fragmentShader) glAttachShader(programObject, fragmentShader);

	// try linking the program with given attachments
	glLinkProgram(programObject);

	// retrieve link status
	GLint status;
	glGetProgramiv(programObject, GL_LINK_STATUS, &status);
	if (status == GL_FALSE)
	{
		GLint length;
		glGetProgramiv(programObject, GL_INFO_LOG_LENGTH, &length);
		string info(length, ' ');
		glGetProgramInfoLog(programObject, info.length(), &length, &info[0]);
		cout << "ERROR linking shader program:" << endl;
		cout << info << endl;
	}

	return programObject;
}
