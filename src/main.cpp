#include "GLFW/glfw3.h"
#include "glhck/glhck.h"

#include "glfwcontext.h"
#include "game.h"

#include <iostream>
#include <fstream>
#include <cstdlib>

int const WINDOW_WIDTH = 800;
int const WINDOW_HEIGHT = 480;

bool RUNNING = true;

void errorCallback(int code, char const* message);
void windowCloseCallback(GLFWwindow* window);
void windowSizeCallback(GLFWwindow *handle, int width, int height);
void gameloop(glfwContext& ctx);
int main(int argc, char** argv)
{
  if (!glfwInit())
  {
    std::cerr << "GLFW initialization error" << std::endl;
    return EXIT_FAILURE;
  }
  glfwSetErrorCallback(errorCallback);

  glhckCompileFeatures features;
  glhckGetCompileFeatures(&features);
  glfwDefaultWindowHints();
  glfwWindowHint(GLFW_SAMPLES, 4);
  if(features.render.glesv1 || features.render.glesv2) {
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_DEPTH_BITS, 16);
  }
  if(features.render.glesv2) {
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
  }
  if(features.render.opengl) {
    glfwWindowHint(GLFW_DEPTH_BITS, 24);
  }

  GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "qb", NULL, NULL);

  if(!window)
  {
    return EXIT_FAILURE;
  }

  glfwContext ctx(window);

  glfwSetWindowUserPointer(window, &ctx);

  glfwMakeContextCurrent(window);

  glfwSetWindowCloseCallback(window, windowCloseCallback);
  glfwSetWindowSizeCallback(window, windowSizeCallback);

  glfwSwapInterval(1);

  if(!glhckContextCreate(argc, argv))
  {
    std::cerr << "Failed to create a GLhck context" << std::endl;
    return EXIT_FAILURE;
  }

  glhckLogColor(0);
  if(!glhckDisplayCreate(WINDOW_WIDTH, WINDOW_HEIGHT, GLHCK_RENDER_AUTO))
  {
    std::cerr << "Failed to create a GLhck display" << std::endl;
    return EXIT_FAILURE;
  }

  glhckRenderClearColorb(64, 64, 64, 255);

  gameloop(ctx);

  glhckContextTerminate();
  glfwTerminate();
  return EXIT_SUCCESS;
}

void errorCallback(int code, char const* message)
{
  std::cerr << "GLFW ERROR: " << message << std::endl;
}

void windowCloseCallback(GLFWwindow* window)
{
  glfwContext* ctx = static_cast<glfwContext*>(glfwGetWindowUserPointer(window));
  ctx->running = false;
}

void windowSizeCallback(GLFWwindow *window, int width, int height)
{
  glhckDisplayResize(width, height);
}

void gameloop(glfwContext& ctx)
{
  float const FPS_INTERVAL = 5.0f;
  float const START_TIME = glfwGetTime();

  std::ifstream ifs("levels/AlbertoG_Plus2.txt");
  std::string line;
  // TODO: Read level pack name
  while(getline(ifs, line) && !line.empty());

  // TODO: Read level pack description
  while(getline(ifs, line) && !line.empty());

  Level* level = loadLevel(ifs);
  Game* game = newGame(level);

  while(ctx.running)
  {
    float const frameStartTime = glfwGetTime();
    ctx.deltaTime = frameStartTime - ctx.previousFrameStartTime;
    ctx.fpsTime += ctx.deltaTime;
    ctx.totalTime = frameStartTime - START_TIME;

    if(ctx.fpsTime >= FPS_INTERVAL)
    {
      ctx.fps = ctx.fpsFrame / ctx.fpsTime;
      ctx.fpsFrame = 0;
      ctx.fpsTime = 0.0f;
    }

    glfwPollEvents();

    playGame(game, ctx);

    glhckRender();

    if(gameFinished(game))
    {
      endGame(game);
      freeLevel(level);
      level = loadLevel(ifs);
      game = newGame(level);
    }

    float const frameEndTime = glfwGetTime();
    ctx.previousFrameDuration = frameEndTime - frameStartTime;

    glfwSwapBuffers(ctx.window);
    ctx.previousFrameStartTime = frameStartTime;
    ctx.frame += 1;
    ctx.fpsFrame += 1;
  }

  endGame(game);
  freeLevel(level);
}
