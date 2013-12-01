#ifndef GLFWCONTEXT_H
#define GLFWCONTEXT_H

#include "GLFW/glfw3.h"

struct glfwContext
{
  glfwContext(GLFWwindow* window) : running(true), window(window),
    totalTime(0.0f), deltaTime(0.0f), fps(0.0f), fpsTime(0.0f), fpsFrame(0),
    previousFrameStartTime(0.0f), previousFrameDuration(0.0f), frame(0)
  {
  }

  bool running;
  GLFWwindow* window;
  float totalTime;
  float deltaTime;
  float fps;
  float fpsTime;
  unsigned int fpsFrame;
  float previousFrameStartTime;
  float previousFrameDuration;
  unsigned long int frame;
};


#endif // GLFWCONTEXT_H
