/*
 * simple.cpp - simple demo for FTGL, the OpenGL font library
 *
 * Copyright (c) 2008 Sam Hocevar <sam@hocevar.net>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "config.h"

#include <math.h> // sin(), cos()
#include <stdlib.h> // exit()

#include <FTGL/ftgl.h>
#define GLFW_INCLUDE_GLCOREARB // Tell GLFW to use gl3.h
#include <GLFW/glfw3.h>


#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static GLuint shaderProgram; 
static GLint vertexCoordAttribute;
static GLint vertexNormalAttribute;
static GLint vertexOffsetUniform;
static GLint mvpUniform;

void compileShaders();
void print_log(GLuint object);

static FTFont *font;
static int lastfps = 0;
static int frames = 0;

static GLFWwindow *window;

//
//  Main OpenGL loop: set up lights, apply a few rotation effects, and
//  render text using the current FTGL object.
//
static void RenderScene(void)
{
    glfwPollEvents();
    float now = glfwGetTime();
    
    glClearColor(1,1,1,1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);


    int screen_width = 1;
    int screen_height = 1;
    float angle = now * 45;
    glm::vec3 axis_y(0, 1, 0);
    glm::mat4 anim = glm::rotate(glm::mat4(1.0f), glm::radians(angle), axis_y);

    glm::mat4 scale = glm::scale(glm::mat4(1.0f), glm::vec3(0.01, 0.01, 0.01));
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(0.0, 0.0, -4.0));

    glm::mat4 view = glm::lookAt(glm::vec3(0.0, 2.0, 0.0), glm::vec3(0.0, 0.0, -4.0), glm::vec3(0.0, 1.0, 0.0));
    glm::mat4 projection = glm::perspective(45.0f, 1.0f*screen_width/screen_height, 0.1f, 10.0f);
    
    glm::mat4 mvp = projection * view * model * scale * anim;

    glUseProgram(shaderProgram);
    glUniformMatrix4fv(mvpUniform, 1, GL_FALSE, glm::value_ptr(mvp));

    font->Render("Great Job!");

    glfwSwapBuffers(window);

    frames++;

    if(now - lastfps > 5000)
    {
        fprintf(stderr, "%i frames in 5.0 seconds = %g FPS\n",
                frames, frames * 1000. / (now - lastfps));
        lastfps += 5000;
        frames = 0;
    }
}

void onReshape(GLFWwindow* win, int width, int height) {
    glViewport(0, 0, width, height);
}

void error_callback(int error, const char* description)
{
    fputs(description, stderr);
}

//
//  Main program entry point: set up GLUT window, load fonts, run GLUT loop.
//
int main(int argc, char **argv)
{
    // From config.h
    char const *file = FONT_FILE;

    // Initialise GLFW
    if(!glfwInit()) {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        return -1;
    }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwSetErrorCallback(error_callback);
    window = glfwCreateWindow( 640, 480, "FTGL GL3 Test", NULL, NULL);
    if( window == NULL ){
        //fprintf( stderr, "Failed to open GLFW window. \n" );
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    
    glfwSetFramebufferSizeCallback(window, onReshape);

    compileShaders();
    
    glEnable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Initialise FTGL stuff
    font = new FTExtrudeFont(file);

    if(font->Error())
    {
        fprintf(stderr, "%s: could not load font `%s'\n", argv[0], file);
        return EXIT_FAILURE;
    }

    font->ShaderLocations(vertexCoordAttribute, vertexNormalAttribute, vertexOffsetUniform);
    font->FaceSize(90);
    font->Depth(100);
    font->Outset(1, 4);
    font->CharMap(ft_encoding_unicode);

    while (!glfwWindowShouldClose(window) && glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS) {
        RenderScene();
    }

    return EXIT_SUCCESS;
}





















static const char * vs_source[] =
{
"#version 150                                                   \n"
"                                                               \n"
"uniform mat4 mvp;                                              \n"
"uniform vec3 pen;                                              \n"
"                                                               \n"
"in      vec3 v_coord;                                        \n"
"in      vec3 v_normal;                                         \n"
"out     vec3 f_color;                                          \n"
"                                                               \n"
"void main(void) {                                              \n"
"  gl_Position = mvp * (vec4(v_coord, 1.0) + vec4(pen, 1.0)); \n"
// "  f_color = vec3(-v_coord.z/100,0.4,0.9);                    \n"
"  f_color = vec3((v_normal.x+1)/2,(v_normal.y+1)/2,(v_normal.z+1)/2);  \n"
"}                                                              \n"
};

static const char * fs_source[] =
{
"#version 150                                                   \n"
"                                                               \n"
"in      vec3 f_color;                                          \n"
"out     vec4 fragColor;                                        \n"
"                                                               \n"
"void main(void) {                                              \n"
"  fragColor = vec4(f_color.x, f_color.y, f_color.z, 1.0);      \n"
"}                                                              \n"
};


void compileShaders() {
    
    printf("Compiling shaders...\n");

    GLint link_ok = GL_FALSE;
 
    GLuint vs = glCreateShader(GL_VERTEX_SHADER); 
    GLuint fs = glCreateShader(GL_FRAGMENT_SHADER);

    glShaderSource(vs, 1, (const GLchar**)&vs_source, 0);
    glShaderSource(fs, 1, (const GLchar**)&fs_source, 0);
    
    /* Compile our shader objects */
    glCompileShader(vs);
    GLint compile_ok = GL_FALSE;
    glGetShaderiv(vs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE) {
        fprintf(stderr, "vertex shader:");
        print_log(vs);
        glDeleteShader(vs);
        return;
    }
    glCompileShader(fs);
    compile_ok = GL_FALSE;
    glGetShaderiv(fs, GL_COMPILE_STATUS, &compile_ok);
    if (compile_ok == GL_FALSE) {
        fprintf(stderr, "fragment shader:");
        print_log(fs);
        glDeleteShader(fs);
        return;
    }
    
    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &link_ok);
    if (!link_ok) {
      fprintf(stderr, "glLinkProgram:");
      print_log(shaderProgram);
      return;
    }
    
    const char* attribute_name;
    attribute_name = "v_coord";
    vertexCoordAttribute = glGetAttribLocation(shaderProgram, attribute_name);
    if (vertexCoordAttribute == -1) {
      fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
      return;
    }


    attribute_name = "v_normal";
    vertexNormalAttribute = glGetAttribLocation(shaderProgram, attribute_name);
    if (vertexNormalAttribute == -1) {
      fprintf(stderr, "Could not bind attribute %s\n", attribute_name);
      return;
    }
    
    const char* uniform_name;
    uniform_name = "mvp";
    mvpUniform = glGetUniformLocation(shaderProgram, uniform_name);
    if (mvpUniform == -1) {
      fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
      return;
    }

    uniform_name = "pen";
    vertexOffsetUniform = glGetUniformLocation(shaderProgram, uniform_name);
    if (vertexOffsetUniform == -1) {
      fprintf(stderr, "Could not bind uniform %s\n", uniform_name);
      return;
    }
}

void print_log(GLuint object)
{
  GLint log_length = 0;
  if (glIsShader(object))
    glGetShaderiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else if (glIsProgram(object))
    glGetProgramiv(object, GL_INFO_LOG_LENGTH, &log_length);
  else {
    fprintf(stderr, "printlog: Not a shader or a program\n");
    return;
  }
 
  char* log = (char*)malloc(log_length);
 
  if (glIsShader(object))
    glGetShaderInfoLog(object, log_length, NULL, log);
  else if (glIsProgram(object))
    glGetProgramInfoLog(object, log_length, NULL, log);
 
  fprintf(stderr, "%s", log);
  free(log);
}