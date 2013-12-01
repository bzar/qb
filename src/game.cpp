#include "game.h"
#include "glhck/glhck.h"
#include "gas.h"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>

float const GRID_SIZE = 1.0f;

enum Direction { UP, DOWN, LEFT, RIGHT };

struct Coordinates {
  int x;
  int y;
};

Coordinates DIRECTIONS[] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};
float DIRECTION_ANGLES[] = {0, 180, 90, 270};

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
    FLOOR, WALL, TARGET
  };

  Type type;
  Object object;
  Coordinates coordinates;
  glhckObject* o;
};

struct Animation
{
  glhckObject* o;
  gasAnimation* a;
};

struct Game
{
  glhckCamera* camera;
  std::vector<std::vector<Tile>> level;
  unsigned int levelWidth;
  unsigned int levelHeight;
  std::vector<Animation> animations;
};

Tile newFloorTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, -GRID_SIZE, y * GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 128, (x + y) % 2 ? 224 : 192, 128, 255);
  Tile tile { Tile::FLOOR, NO_OBJECT, {x, y}, o };
  return tile;
}

Tile newWallTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, 0, y * GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 96, 96, 96, 255);
  Tile tile { Tile::WALL, NO_OBJECT, {x, y}, o };
  return tile;
}
Tile newTargetTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, -GRID_SIZE, y * GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 192, 255, 192, 64);
  Tile tile { Tile::TARGET, NO_OBJECT, {x, y}, o };
  return tile;
}

Object newPlayerObject(int x, int y)
{
  //glhckObject* o = glhckCubeNew(GRID_SIZE / 3.0f);
  glhckImportModelParameters animatedParams = *glhckImportDefaultModelParameters();
  animatedParams.animated = 1;

  glhckObject* o = glhckModelNew("model/player.glhckm", GRID_SIZE, &animatedParams);
  //glhckObjectDrawAABB(o, 1);
  glhckObjectPositionf(o, x * GRID_SIZE, -0.5, y * GRID_SIZE);
  unsigned int numAnimations = 0;
  glhckAnimation** animations = glhckObjectAnimations(o, &numAnimations);
  std::cout << "Total animations: " << numAnimations << std::endl;
  for(int i = 0; i < numAnimations; ++i) {
    std::cout << "Animation " << i << ": " << glhckAnimationGetName(animations[i]) << std::endl;
  }
  //glhckObjectMaterial(o, glhckMaterialNew(NULL));
  //glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 96, 96, 192, 255);

  Object object { Object::PLAYER, o };
  return object;
}

Object newBoxObject(int x, int y)
{
  glhckObject* o = glhckCubeNew(2 * GRID_SIZE / 5.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, 0, y * GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(NULL));
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), 192, 192, 64, 255);
  Object object { Object::BOX, o };
  return object;
}

bool gameWon(Game* game)
{
  for(std::vector<Tile>& rows : game->level)
  {
    for(Tile& tile : rows)
    {
      if(tile.type == Tile::TARGET && tile.object.type != Object::BOX)
      {
        return false;
      }
    }
  }

  return true;
}

Coordinates findPlayer(Game* game)
{
  for(std::vector<Tile>& rows : game->level)
  {
    for(Tile& tile : rows)
    {
      if(tile.object.type == Object::PLAYER)
      {
        return tile.coordinates;
      }
    }
  }

  return {-1, -1};
}

Tile& getTile(Game* game, int x, int y)
{
  return game->level.at(y).at(x);
}

void move(Game* game, Direction direction)
{
  Coordinates current = findPlayer(game);
  Coordinates& delta = DIRECTIONS[direction];
  Coordinates destination = {current.x + delta.x, current.y + delta.y};
  Tile& currentTile = getTile(game, current.x, current.y);
  Tile& destinationTile = getTile(game, destination.x, destination.y);

  if(destinationTile.type == Tile::WALL)
  {
    return;
  }

  if(destinationTile.object.type != Object::NONE)
  {
    Coordinates pushDestination = {destination.x + delta.x, destination.y + delta.y};
    Tile& pushDestinationTile = getTile(game, pushDestination.x, pushDestination.y);
    if(pushDestinationTile.type == Tile::WALL || pushDestinationTile.object.type != Object::NONE)
    {
      return;
    }

    gasAnimation* parts[] = {
      gasNumberAnimationNewTo(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, pushDestination.x * GRID_SIZE, 0.5),
      gasNumberAnimationNewTo(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, pushDestination.y * GRID_SIZE, 0.5)
    };
    Animation animation = {
      destinationTile.object.o,
      gasParallelAnimationNew(parts, 2)
    };
    game->animations.push_back(animation);
    pushDestinationTile.object = destinationTile.object;
    destinationTile.object = NO_OBJECT;
  }

  float angle = glhckObjectGetRotation(currentTile.object.o)->y;
  float dir = DIRECTION_ANGLES[direction];
  float a = fabs(dir - angle) > fabs((360 - dir) - angle) ? 360 - dir: dir;
  gasAnimation* parts[] = {
    gasNumberAnimationNewTo(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, destination.x * GRID_SIZE, 0.5),
    gasNumberAnimationNewTo(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, destination.y * GRID_SIZE, 0.5),
    gasNumberAnimationNewTo(GAS_NUMBER_ANIMATION_TARGET_ROT_Y, GAS_EASING_LINEAR, a, 0.25),
    gasModelAnimationNew(currentTile.object.o, "Walk", 0.5)
  };
  Animation animation = {
    currentTile.object.o,
    gasParallelAnimationNew(parts, 4)
  };
  game->animations.push_back(animation);
  destinationTile.object = currentTile.object;
  currentTile.object = NO_OBJECT;
}

Game* newGame()
{
  Game* game = new Game;
  std::vector<std::string> rows {
    "############",
    "#..........#",
    "#....x.....#",
    "#..........#",
    "#...###....#",
    "#..........#",
    "#..@..B....#",
    "#..........#",
    "#..........#",
    "#x...B.....#",
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
      else if(c == 'x')
      {
        levelRow.push_back(newTargetTile(x, y));
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
  glhckCameraProjection(game->camera, GLHCK_PROJECTION_PERSPECTIVE);
  glhckObjectPositionf(glhckCameraGetObject(game->camera),
                       game->levelWidth * GRID_SIZE / 2,
                       game->levelWidth * GRID_SIZE,
                       -(game->levelWidth * GRID_SIZE / 2));
  glhckObjectTargetf(glhckCameraGetObject(game->camera),
                     game->levelWidth * GRID_SIZE / 2,
                     0,
                     game->levelHeight * GRID_SIZE / 2);

  glhckCameraRange(game->camera, 0.1f, 100.0f);
  glhckCameraFov(game->camera, 45);
  glhckCameraUpdate(game->camera);

  return game;
}


void playGame(Game* game, glfwContext& ctx)
{
  if(game->animations.empty())
  {
    if(gameWon(game))
    {
      ctx.running = false;
    }
    else if(glfwGetKey(ctx.window, GLFW_KEY_UP))
    {
      move(game, UP);
    }
    else if(glfwGetKey(ctx.window, GLFW_KEY_DOWN))
    {
      move(game, DOWN);
    }
    else if(glfwGetKey(ctx.window, GLFW_KEY_LEFT))
    {
      move(game, LEFT);
    }
    else if(glfwGetKey(ctx.window, GLFW_KEY_RIGHT))
    {
      move(game, RIGHT);
    }
  }
  else
  {
    for(Animation& animation : game->animations)
    {
      gasAnimate(animation.a, animation.o, ctx.deltaTime);

      if(gasAnimationGetState(animation.a) == GAS_ANIMATION_STATE_FINISHED)
      {
        gasAnimationFree(animation.a);
        animation.a = nullptr;
      }
    }

    auto removeIter = std::remove_if(game->animations.begin(), game->animations.end(), [](Animation& animation) {
      return animation.a == nullptr;
    });
    game->animations.erase(removeIter, game->animations.end());
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
