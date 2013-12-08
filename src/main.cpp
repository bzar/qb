#include "GLFW/glfw3.h"
#include "glhck/glhck.h"

#include "glfwcontext.h"
#include "game.h"

#include <cstdlib>

int const WINDOW_WIDTH = 800;
int const WINDOW_HEIGHT = 480;

bool RUNNING = true;

void windowCloseCallback(GLFWwindow* window);
void windowSizeCallback(GLFWwindow *handle, int width, int height);
void gameloop(glfwContext& ctx);

int main(int argc, char** argv)
{
  if (!glfwInit())
    return EXIT_FAILURE;

  glfwWindowHint(GLFW_DEPTH_BITS, 24);
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
    return EXIT_FAILURE;
  }

  glhckLogColor(0);
  if(!glhckDisplayCreate(WINDOW_WIDTH, WINDOW_HEIGHT, GLHCK_RENDER_AUTO))
  {
    return EXIT_FAILURE;
  }

  glhckRenderClearColorb(255, 0, 255, 255);

  gameloop(ctx);

  glhckContextTerminate();
  glfwTerminate();
  return EXIT_SUCCESS;
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

  Game* game = newGame();

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
    float const frameEndTime = glfwGetTime();
    ctx.previousFrameDuration = frameEndTime - frameStartTime;

    glfwSwapBuffers(ctx.window);
    ctx.previousFrameStartTime = frameStartTime;
    ctx.frame += 1;
    ctx.fpsFrame += 1;
  }

  endGame(game);
}
