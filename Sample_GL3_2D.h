#include <iostream>
#include <cmath>
#include <fstream>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include <FTGL/ftgl.h>
// #include <SOIL/SOIL.h>
#include "SOIL.h"
#include "keyboard_mouse.h"

#define GLM_FORCE_RADIANS
#include <glm/glm.hpp>
#include <glm/gtx/transform.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define PI 3.141592653589
#define DEG2RAD(deg) (deg * PI / 180)