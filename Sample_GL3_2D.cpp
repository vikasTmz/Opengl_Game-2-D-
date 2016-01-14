  #include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)

using namespace std;

struct VAO {
  GLuint VertexArrayID;
  GLuint VertexBuffer;
  GLuint ColorBuffer;

  GLenum PrimitiveMode;
  GLenum FillMode;
  int NumVertices;
};
typedef struct VAO VAO;

struct GLMatrices {
	glm::mat4 projection;
	glm::mat4 model;
	glm::mat4 view;
	GLuint MatrixID;
} Matrices;

GLuint programID;

GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path);
void drawBall(float rad) ;
void quit(GLFWwindow *window);


float x_triangle = -2.0f, x_circle=1,y_circle=3, gravity=0.98,vx_circle=0,vy_circle=0;

float x_circle2=-30,y_circle2=-30,v_circle2 = 0.11 ,circle2_angle=86, vxi_circle2 = v_circle2*cos(DEG2RAD(circle2_angle)),vyi_circle2=v_circle2*sin(DEG2RAD(circle2_angle));
float vx_circle2,vy_circle2;

float camera_rotation_angle = 90, rectangle_rotation = 0, triangle_rotation = 0, currtime,mass_circle=0.7;

static void error_callback(int error, const char* description)
{
  fprintf(stderr, "Error: %s\n", description);
}



/* Generate VAO, VBOs and return VAO handle */
struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat* color_buffer_data, GLenum fill_mode=GL_FILL)
{
  struct VAO* vao = new struct VAO;
  vao->PrimitiveMode = primitive_mode;
  vao->NumVertices = numVertices;
  vao->FillMode = fill_mode;

    // Create Vertex Array Object
    // Should be done after CreateWindow and before any other GL calls
    glGenVertexArrays(1, &(vao->VertexArrayID)); // VAO
    glGenBuffers (1, &(vao->VertexBuffer)); // VBO - vertices
    glGenBuffers (1, &(vao->ColorBuffer));  // VBO - colors

    glBindVertexArray (vao->VertexArrayID); // Bind the VAO 
    glBindBuffer (GL_ARRAY_BUFFER, vao->VertexBuffer); // Bind the VBO vertices 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), vertex_buffer_data, GL_STATIC_DRAW); // Copy the vertices into VBO
    glVertexAttribPointer(
                          0,                  // attribute 0. Vertices
                          3,                  // size (x,y,z)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    glBindBuffer (GL_ARRAY_BUFFER, vao->ColorBuffer); // Bind the VBO colors 
    glBufferData (GL_ARRAY_BUFFER, 3*numVertices*sizeof(GLfloat), color_buffer_data, GL_STATIC_DRAW);  // Copy the vertex colors
    glVertexAttribPointer(
                          1,                  // attribute 1. Color
                          3,                  // size (r,g,b)
                          GL_FLOAT,           // type
                          GL_FALSE,           // normalized?
                          0,                  // stride
                          (void*)0            // array buffer offset
                          );

    return vao;
  }

/* Generate VAO, VBOs and return VAO handle - Common Color for all vertices */
  struct VAO* create3DObject (GLenum primitive_mode, int numVertices, const GLfloat* vertex_buffer_data, const GLfloat red, const GLfloat green, const GLfloat blue, GLenum fill_mode=GL_FILL)
  {
    GLfloat* color_buffer_data = new GLfloat [3*numVertices];
    for (int i=0; i<numVertices; i++) {
      color_buffer_data [3*i] = red;
      color_buffer_data [3*i + 1] = green;
      color_buffer_data [3*i + 2] = blue;
    }

    return create3DObject(primitive_mode, numVertices, vertex_buffer_data, color_buffer_data, fill_mode);
  }

/* Render the VBOs handled by VAO */
  void draw3DObject (struct VAO* vao);


  float triangle_rot_dir = 1;
  float rectangle_rot_dir = 1;
  bool triangle_rot_status = true;
  bool rectangle_rot_status = true;

/* Executed when a regular key is pressed/released/held-down */
/* Prefered for Keyboard events */
  void keyboard (GLFWwindow* window, int key, int scancode, int action, int mods)
  {
     // Function is called first on GLFW_PRESS.

    if (action == GLFW_RELEASE) {
      switch (key) {
        case GLFW_KEY_C:
        rectangle_rot_status = !rectangle_rot_status;
        break;
        case GLFW_KEY_P:
        triangle_rot_status = !triangle_rot_status;
        break;
        case GLFW_KEY_D:
        x_triangle += 0.1f;
        break;
        default:
        break;
      }
    }

    if(action == GLFW_REPEAT)
    {
     switch (key) {
      case GLFW_KEY_A:
      x_triangle -= 0.1f;
      break;
      case GLFW_KEY_D:
      x_triangle += 0.1f;
      break;
      default:
      break;
    }
  }

  else if (action == GLFW_PRESS) {
    switch (key) {
      case GLFW_KEY_ESCAPE:
      quit(window);
      break;
      default:
      break;
    }
  }
}

/* Executed for character input (like in text boxes) */
void keyboardChar (GLFWwindow* window, unsigned int key)
{
	switch (key) {
		case 'Q':
		case 'q':
    quit(window);
    break;
    default:
    break;
  }
}

/* Executed when a mouse button is pressed/released */
void mouseButton (GLFWwindow* window, int button, int action, int mods)
{
  switch (button) {
    case GLFW_MOUSE_BUTTON_LEFT:
    if (action == GLFW_RELEASE)
      triangle_rot_dir *= -1;
    break;
    case GLFW_MOUSE_BUTTON_RIGHT:
    if (action == GLFW_RELEASE) {
      rectangle_rot_dir *= -1;
    }
    break;
    default:
    break;
  }
}


void reshapeWindow (GLFWwindow* window, int width, int height)
{
  int fbwidth=width, fbheight=height;
  glfwGetFramebufferSize(window, &fbwidth, &fbheight);

  GLfloat fov = 90.0f;

  glViewport (0, 0, (GLsizei) fbwidth, (GLsizei) fbheight);

  Matrices.projection = glm::ortho(-40.0f, 40.0f, -40.0f, 40.0f, 0.1f, 500.0f);
}

VAO *triangle, *rectangle, *TRI, *Circle;

void createTriangle ()
{
  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    -1,-1,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 0
    0,1,0, // color 1
    0,0,1, // color 2
  };

  triangle = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);

}

void createTRI ()
{

  static const GLfloat vertex_buffer_data [] = {
    0, 1,0, // vertex 0
    1,0,0, // vertex 1
    1,-1,0, // vertex 2
  };

  static const GLfloat color_buffer_data [] = {
    1,1,0, // color 0
    0,1,0, // color 1
    1,0,1, // color 2
  };

  TRI = create3DObject(GL_TRIANGLES, 3, vertex_buffer_data, color_buffer_data, GL_FILL);
}



void createRectangle ()
{
  // GL3 accepts only Triangles. Quads are not supported
  static const GLfloat vertex_buffer_data [] = {
    -1.2,-1,0, // vertex 1
    1.2,-1,0, // vertex 2
    1.2, 1,0, // vertex 3

    1.2, 1,0, // vertex 3
    -1.2, 1,0, // vertex 4
    -1.2,-1,0  // vertex 1
  };

  static const GLfloat color_buffer_data [] = {
    1,0,0, // color 1
    0,0,1, // color 2
    0,1,0, // color 3

    0,1,0, // color 3
    0.3,0.3,0.3, // color 4
    1,0,0  // color 1
  };

  // create3DObject creates and returns a handle to a VAO that can be used later
  rectangle = create3DObject(GL_TRIANGLES, 6, vertex_buffer_data, color_buffer_data, GL_FILL);
}
void drawBall(float rad)
{
  int i,k=0;
  GLfloat vertex_buffer_data[1090]={};
  GLfloat color_buffer_data[1090]={};
  for (i = 0; i < 360; ++i)
  {
   vertex_buffer_data[k] = rad*cos(DEG2RAD(i));
   color_buffer_data[k] = rand()%2;
       // cout << ((double) rand() / (RAND_MAX)) << endl;

   k++;
   vertex_buffer_data[k] = rad*sin(DEG2RAD(i));
   color_buffer_data[k] = rand()%2;
   k++;
   vertex_buffer_data[k] = 0;
   color_buffer_data[k] = rand()%2;
   k++;
 }

 Circle = create3DObject(GL_TRIANGLE_FAN, 360, vertex_buffer_data, color_buffer_data, GL_FILL);
}

int fall_flag = 0,no_bounces=1;
void Update()
{
  if(y_circle > -6.5f && vy_circle <= 0 )
  {
    if(fall_flag == 0)
    {
      vy_circle = -vy_circle;
      fall_flag = 1;
    }
    currtime = glfwGetTime();
    // cout <<  "Fall  " << vy_circle  << "   flag  " << fall_flag << endl;
    vy_circle -= (gravity) * (currtime*0.08);
    y_circle += vy_circle * (currtime*0.08);
  }
  else if(vy_circle > 0 || fall_flag == 1)
  {

    if(fall_flag == 1)
    {
      vy_circle = -vy_circle*mass_circle;
      fall_flag =0;
    }
    currtime = glfwGetTime();
    // cout <<  "Rise  " << vy_circle << "   flag " << fall_flag << endl;
    vy_circle -= (gravity) * (currtime*0.08);
    y_circle += vy_circle * (currtime*0.08);
  }
}
void projectile()
{
    currtime = glfwGetTime();
    vy_circle2 = vyi_circle2 - (gravity)*(currtime*0.08);
    vx_circle2 = vxi_circle2;
    x_circle2 += vx_circle2;
    y_circle2 += vy_circle2 ;
    no_bounces++; 
    // x_circle2 = vx_circle2*(currtime);
    // y_circle2 = vy_circle*(currtime) -0.5*gravity*(currtime)*(currtime);
    if(y_circle2 < -30.2)
    {
      vy_circle2 = -vy_circle2;
      cout << "vyb:  " <<  y_circle2 << endl;
    }

}

void draw ()
{
  Update();
  projectile();
  glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  glUseProgram (programID);

  glm::vec3 eye ( 5*cos(camera_rotation_angle*M_PI/180.0f), 0, 5*sin(camera_rotation_angle*M_PI/180.0f) );
  // Target - Where is the camera looking at.  Don't change unless you are sure!!
  glm::vec3 target (0, 0, 0);
  // Up - Up vector defines tilt of camera.  Don't change unless you are sure!!
  glm::vec3 up (0, 1, 0);

  Matrices.view = glm::lookAt(glm::vec3(0,0,3), glm::vec3(0,0,0), glm::vec3(0,1,0)); // Fixed camera for 2D (ortho) in XY plane

  glm::mat4 VP = Matrices.projection * Matrices.view;

  glm::mat4 MVP;	// MVP = Projection * View * Model

  // Load identity to model matrix
  Matrices.model = glm::mat4(1.0f);


  glm::mat4 translateTriangle = glm::translate (glm::vec3(x_triangle, 0.0f, 0.0f)); // glTranslatef
  glm::mat4 rotateTriangle = glm::rotate((float)(triangle_rotation*M_PI/180.0f), glm::vec3(0,0,1));  // rotate about vector (1,0,0)
  glm::mat4 triangleTransform = translateTriangle * rotateTriangle;
  Matrices.model *= triangleTransform; 
  MVP = VP * Matrices.model; // MVP = p * V * M
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(triangle);

  Matrices.model = glm::mat4(1.0f);

  // glm::mat4 translateRectangle = glm::translate (glm::vec3(2, 0, 0));        // glTranslatef
  // glm::mat4 rotateRectangle = glm::rotate((float)(rectangle_rotation*M_PI/120.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  // Matrices.model *= (translateRectangle * rotateRectangle);
  // MVP = VP * Matrices.model;
  // glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  // draw3DObject(rectangle);

  ///////////////////////////CIRCLE///////////////////////
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateCircle = glm::translate (glm::vec3(x_circle, y_circle, 0));        // glTranslatef
  glm::mat4 rotateCircle = glm::rotate((float)(rectangle_rotation*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateCircle * rotateCircle);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(Circle); 

    ///////////////////////////CIRCLE///////////////////////
  Matrices.model = glm::mat4(1.0f);
  glm::mat4 translateCircle2 = glm::translate (glm::vec3(x_circle2, y_circle2, 0));        // glTranslatef
  glm::mat4 rotateCircle2 = glm::rotate((float)(0*M_PI/180.0f), glm::vec3(0,0,1)); // rotate about vector (-1,1,1)
  Matrices.model *= (translateCircle2 * rotateCircle2);
  MVP = VP * Matrices.model;
  glUniformMatrix4fv(Matrices.MatrixID, 1, GL_FALSE, &MVP[0][0]);
  draw3DObject(Circle);  


  float increments = 1;

  //camera_rotation_angle++; // Simulating camera rotation
  triangle_rotation = triangle_rotation + increments*triangle_rot_dir*triangle_rot_status;
  rectangle_rotation = rectangle_rotation + increments*rectangle_rot_dir*rectangle_rot_status;
}


GLFWwindow* initGLFW (int width, int height)
{
    GLFWwindow* window; // window desciptor/handle

    glfwSetErrorCallback(error_callback);
    if (!glfwInit()) {
      exit(EXIT_FAILURE);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(width, height, "Sample OpenGL 3.3 Application", NULL, NULL);

    if (!window) {
      glfwTerminate();
      exit(EXIT_FAILURE);
    }

    glfwMakeContextCurrent(window);
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);
    glfwSwapInterval( 1 );

    glfwSetFramebufferSizeCallback(window, reshapeWindow);
    glfwSetWindowSizeCallback(window, reshapeWindow);

    glfwSetWindowCloseCallback(window, quit);

    glfwSetKeyCallback(window, keyboard);      // general keyboard input
    glfwSetCharCallback(window, keyboardChar);  // simpler specific character handling

    glfwSetMouseButtonCallback(window, mouseButton);  // mouse button clicks

    return window;
  }

  void initGL (GLFWwindow* window, int width, int height)
  {
	createTriangle (); // Generate the VAO, VBOs, vertices data & copy into the array buffer
	createRectangle ();
	createTRI();
  drawBall(1.5);

  programID = LoadShaders( "Sample_GL.vert", "Sample_GL.frag" );
  Matrices.MatrixID = glGetUniformLocation(programID, "MVP");


  reshapeWindow (window, width, height);

	glClearColor (0.3f, 0.3f, 0.3f, 0.0f); // R, G, B, A
	glClearDepth (1.0f);

	glEnable (GL_DEPTH_TEST);
	glDepthFunc (GL_LEQUAL);

  cout << "VENDOR: " << glGetString(GL_VENDOR) << endl;
  cout << "RENDERER: " << glGetString(GL_RENDERER) << endl;
  cout << "VERSION: " << glGetString(GL_VERSION) << endl;
  cout << "GLSL: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;
}

int main (int argc, char** argv)
{
	int width = 600;
	int height = 600;

  GLFWwindow* window = initGLFW(width, height);

  initGL (window, width, height);

  double last_update_time = glfwGetTime(), current_time;

    /* Draw in loop */
  while (!glfwWindowShouldClose(window)) 
  {

        // OpenGL Draw commands
    draw();

        // Swap Frame Buffer in double buffering
    glfwSwapBuffers(window);

        // Poll for Keyboard and mouse events
    glfwPollEvents();

        // Control based on time (Time based transformation like 5 degrees rotation every 0.5s)
        current_time = glfwGetTime(); // Time in seconds
        if ((current_time - last_update_time) >= 0.5) 
        { // atleast 0.5s elapsed since last frame
            // do something every 0.5 seconds ..
          last_update_time = current_time;
        }
      }

      glfwTerminate();
      exit(EXIT_SUCCESS);
    }



    GLuint LoadShaders(const char * vertex_file_path,const char * fragment_file_path) {

  // Create the shaders
      GLuint VertexShaderID = glCreateShader(GL_VERTEX_SHADER);
      GLuint FragmentShaderID = glCreateShader(GL_FRAGMENT_SHADER);

  // Read the Vertex Shader code from the file
      std::string VertexShaderCode;
      std::ifstream VertexShaderStream(vertex_file_path, std::ios::in);
      if(VertexShaderStream.is_open())
      {
        std::string Line = "";
        while(getline(VertexShaderStream, Line))
          VertexShaderCode += "\n" + Line;
        VertexShaderStream.close();
      }

  // Read the Fragment Shader code from the file
      std::string FragmentShaderCode;
      std::ifstream FragmentShaderStream(fragment_file_path, std::ios::in);
      if(FragmentShaderStream.is_open()){
        std::string Line = "";
        while(getline(FragmentShaderStream, Line))
          FragmentShaderCode += "\n" + Line;
        FragmentShaderStream.close();
      }

      GLint Result = GL_FALSE;
      int InfoLogLength;

  // Compile Vertex Shader
      printf("Compiling shader : %s\n", vertex_file_path);
      char const * VertexSourcePointer = VertexShaderCode.c_str();
      glShaderSource(VertexShaderID, 1, &VertexSourcePointer , NULL);
      glCompileShader(VertexShaderID);

  // Check Vertex Shader
      glGetShaderiv(VertexShaderID, GL_COMPILE_STATUS, &Result);
      glGetShaderiv(VertexShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
      std::vector<char> VertexShaderErrorMessage(InfoLogLength);
      glGetShaderInfoLog(VertexShaderID, InfoLogLength, NULL, &VertexShaderErrorMessage[0]);
      fprintf(stdout, "%s\n", &VertexShaderErrorMessage[0]);

  // Compile Fragment Shader
      printf("Compiling shader : %s\n", fragment_file_path);
      char const * FragmentSourcePointer = FragmentShaderCode.c_str();
      glShaderSource(FragmentShaderID, 1, &FragmentSourcePointer , NULL);
      glCompileShader(FragmentShaderID);

  // Check Fragment Shader
      glGetShaderiv(FragmentShaderID, GL_COMPILE_STATUS, &Result);
      glGetShaderiv(FragmentShaderID, GL_INFO_LOG_LENGTH, &InfoLogLength);
      std::vector<char> FragmentShaderErrorMessage(InfoLogLength);
      glGetShaderInfoLog(FragmentShaderID, InfoLogLength, NULL, &FragmentShaderErrorMessage[0]);
      fprintf(stdout, "%s\n", &FragmentShaderErrorMessage[0]);

  // Link the program
      fprintf(stdout, "Linking program\n");
      GLuint ProgramID = glCreateProgram();
      glAttachShader(ProgramID, VertexShaderID);
      glAttachShader(ProgramID, FragmentShaderID);
      glLinkProgram(ProgramID);

  // Check the program
      glGetProgramiv(ProgramID, GL_LINK_STATUS, &Result);
      glGetProgramiv(ProgramID, GL_INFO_LOG_LENGTH, &InfoLogLength);
      std::vector<char> ProgramErrorMessage( max(InfoLogLength, int(1)) );
      glGetProgramInfoLog(ProgramID, InfoLogLength, NULL, &ProgramErrorMessage[0]);
      fprintf(stdout, "%s\n", &ProgramErrorMessage[0]);

      glDeleteShader(VertexShaderID);
      glDeleteShader(FragmentShaderID);

      return ProgramID;
    }
    void quit(GLFWwindow *window)
    {
      glfwDestroyWindow(window);
      glfwTerminate();
      exit(EXIT_SUCCESS);
    }
    void draw3DObject (struct VAO* vao)
    {
    // Change the Fill Mode for this object
      glPolygonMode (GL_FRONT_AND_BACK, vao->FillMode);

    // Bind the VAO to use
      glBindVertexArray (vao->VertexArrayID);

    // Enable Vertex Attribute 0 - 3d Vertices
      glEnableVertexAttribArray(0);
    // Bind the VBO to use
      glBindBuffer(GL_ARRAY_BUFFER, vao->VertexBuffer);

    // Enable Vertex Attribute 1 - Color
      glEnableVertexAttribArray(1);
    // Bind the VBO to use
      glBindBuffer(GL_ARRAY_BUFFER, vao->ColorBuffer);

    // Draw the geometry !
    glDrawArrays(vao->PrimitiveMode, 0, vao->NumVertices); // Starting from vertex 0; 3 vertices total -> 1 triangle
  }