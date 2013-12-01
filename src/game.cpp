#include "game.h"
#include "glhck/glhck.h"
#include <vector>
#include <string>
#include <iostream>

float const GRID_SIZE = 1.0f;

struct Object {
  enum Type {
    NONE, PLAYER, BOX
  };

  Type type;
  glhckObject* o;
};

Object const NO_OBJECT = { Object::NONE, nullptr };

struct Tile
{
  enum Type {
    FLOOR, WALL
  };

  Type type;
  Object object;
  glhckObject* o;
};

struct Game
{
  glhckCamera* camera;
  std::vector<std::vector<Tile>> level;
  unsigned int levelWidth;
  unsigned int levelHeight;
};

Tile newFloorTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, y * GRID_SIZE, -GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 128, 224, 128, 255);
  Tile tile { Tile::FLOOR, NO_OBJECT, o };
  return tile;
}

Tile newWallTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, y * GRID_SIZE, 0);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 96, 96, 96, 255);
  Tile tile { Tile::FLOOR, NO_OBJECT, o };
  return tile;
}

Object newPlayerObject(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 3.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, y * GRID_SIZE, 0);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 96, 96, 192, 255);
  Object object { Object::PLAYER, o };
  return object;
}

Object newBoxObject(int x, int y)
{
  glhckObject* o = glhckCubeNew(2 * GRID_SIZE / 5.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, y * GRID_SIZE, 0);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 192, 192, 64, 255);
  Object object { Object::BOX, o };
  return object;
}

Game* newGame()
{
  Game* game = new Game;
  std::vector<std::string> rows {
    "############",
    "#..........#",
    "#..........#",
    "#..........#",
    "#...###....#",
    "#..........#",
    "#..@..B....#",
    "#..........#",
    "#..........#",
    "#....B.....#",
    "#..........#",
    "############",
  };

  game->levelWidth = 0;
  game->levelHeight = 0;

  for(std::string const& row : rows)
  {
    int y = game->level.size();
    game->level.push_back({});
    std::vector<Tile>& levelRow = game->level.back();
    for(char const& c : row)
    {
      int x = levelRow.size();
      if(c == '#')
      {
        levelRow.push_back(newWallTile(x, y));
      }
      else if(c == '.')
      {
        levelRow.push_back(newFloorTile(x, y));
      }
      else if(c == '@')
      {
        Tile tile = newFloorTile(x, y);
        tile.object = newPlayerObject(x, y);
        levelRow.push_back(tile);
      }
      else if(c == 'B')
      {
        Tile tile = newFloorTile(x, y);
        tile.object = newBoxObject(x, y);
        levelRow.push_back(tile);
      }
    }

    if(levelRow.size() > game->levelWidth)
    {
      game->levelWidth = levelRow.size();
    }
  }

  game->levelHeight = game->level.size();

  game->camera = glhckCameraNew();
  glhckCameraProjection(game->camera, GLHCK_PROJECTION_ORTHOGRAPHIC);
  /*glhckObjectPositionf(glhckCameraGetObject(game->camera),
                       game->levelWidth * GRID_SIZE / 2,
                       -game->levelHeight * GRID_SIZE,
                       game->levelWidth * GRID_SIZE*2.5);*/
  glhckObjectTargetf(glhckCameraGetObject(game->camera),
                     game->levelWidth * GRID_SIZE / 2,
                     game->levelHeight * GRID_SIZE / 2,
                     0);
  glhckCameraRange(game->camera, 0.1f, 100.0f);
  glhckCameraFov(game->camera, 45);
  glhckCameraUpdate(game->camera);

  return game;
}


void playGame(Game* game, glfwContext& ctx)
{
  if(glfwGetKey(ctx.window, GLFW_KEY_LEFT) == GLFW_PRESS)
  {
    glhckObjectRotatef(glhckCameraGetObject(game->camera), 0.0f, 0.001f, 0.0f);
  }
  if(glfwGetKey(ctx.window, GLFW_KEY_RIGHT) == GLFW_PRESS)
  {
    glhckObjectRotatef(glhckCameraGetObject(game->camera), 0.0f, -0.001f, 0.0f);
  }
  if(glfwGetKey(ctx.window, GLFW_KEY_UP) == GLFW_PRESS)
  {
    glhckObjectMovef(glhckCameraGetObject(game->camera), 0.0f, 0.0f, 0.1f);
  }
  if(glfwGetKey(ctx.window, GLFW_KEY_DOWN) == GLFW_PRESS)
  {
    glhckObjectMovef(glhckCameraGetObject(game->camera), 0.0f, 0.0f, -0.1f);
  }
  glhckRenderClear(GLHCK_DEPTH_BUFFER | GLHCK_COLOR_BUFFER);
  glhckCameraUpdate(game->camera);

  for(std::vector<Tile>& rows : game->level)
  {
    for(Tile& tile : rows)
    {
      glhckObjectDraw(tile.o);
      if(tile.object.type != Object::NONE)
      {
        glhckObjectDraw(tile.object.o);
      }
    }
  }
}


void endGame(Game* game)
{
  for(std::vector<Tile>& rows : game->level)
  {
    for(Tile& tile : rows)
    {
      glhckObjectFree(tile.o);
      if(tile.object.type != Object::NONE)
      {
        glhckObjectFree(tile.object.o);
      }
    }
  }
  delete game;
}
