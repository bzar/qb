#include "game.h"
#include "glhck/glhck.h"
#include "gasxx.h"

#include <vector>
#include <string>
#include <iostream>
#include <algorithm>
#include <sstream>

float const GRID_SIZE = 1.0f;

enum Direction { UP, DOWN, LEFT, RIGHT };

struct Coordinates {
  int x;
  int y;
};

Coordinates DIRECTIONS[] = {{0, -1}, {0, 1}, {-1, 0}, {1, 0}};
float DIRECTION_ANGLES[] = {180, 0, 270, 90};

struct Object {
  enum Type {
    NONE, PLAYER, BOX
  };

  Type type;
  glhckObject* o;
  Direction facing;
  gas::Animation a;
};

Object const NO_OBJECT = { Object::NONE, nullptr, UP, gas::Animation::NONE };

struct Tile
{
  enum Type {
    NONE, FLOOR, WALL, TARGET
  };

  Type type;
  Object object;
  Coordinates coordinates;
  glhckObject* o;
};

struct Level
{
  std::vector<std::vector<Tile>> tiles;
  unsigned int width;
  unsigned int height;
  std::string name;
};

struct Game
{
  glhckCamera* camera;
  Level level;
  bool animating;
};

glhckObject* texturedCube(float size, std::string const& textureFilename)
{
  glhckObject* o = glhckCubeNew(size);
  glhckTexture* texture = glhckTextureNewFromFile(textureFilename.data(), glhckImportDefaultImageParameters(), glhckTextureDefaultParameters());
  glhckMaterial* material = glhckMaterialNew(texture);
  glhckObjectMaterial(o, material);
  glhckMaterialFree(material);
  glhckTextureFree(texture);
  return o;
}

Tile newEmptyTile(int x, int y)
{
  return { Tile::NONE, NO_OBJECT, {x, y}, nullptr };
}

Tile newFloorTile(int x, int y, Object const& object = NO_OBJECT)
{
  glhckObject* o = texturedCube(GRID_SIZE / 2.0f, "model/floor.jpg");
  glhckObjectPositionf(o, x * GRID_SIZE, -GRID_SIZE, y * GRID_SIZE);
  float brightness = (x + y) % 2 ? 224 : 255;
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), brightness, brightness, brightness, 255);
  Tile tile { Tile::FLOOR, object, {x, y}, o };
  return tile;
}

Tile newWallTile(int x, int y)
{
  glhckObject* o = texturedCube(GRID_SIZE / 2.0f, "model/wall.jpg");
  glhckObjectPositionf(o, x * GRID_SIZE, 0, y * GRID_SIZE);
  Tile tile { Tile::WALL, NO_OBJECT, {x, y}, o };
  return tile;
}
Tile newTargetTile(int x, int y)
{
  glhckObject* o = glhckCubeNew(GRID_SIZE / 2.0f);
  glhckObjectPositionf(o, x * GRID_SIZE, -GRID_SIZE, y * GRID_SIZE);
  glhckObjectMaterial(o, glhckMaterialNew(glhckTextureNewFromFile("model/target.jpg", glhckImportDefaultImageParameters(), glhckTextureDefaultParameters())));
  Tile tile { Tile::TARGET, NO_OBJECT, {x, y}, o };
  return tile;
}

Object newPlayerObject(int x, int y)
{
  glhckImportModelParameters animatedParams = *glhckImportDefaultModelParameters();
  animatedParams.animated = 1;

  glhckObject* o = glhckModelNew("model/pig.glhckm", GRID_SIZE, &animatedParams);
  glhckObjectPositionf(o, x * GRID_SIZE, -0.5, y * GRID_SIZE);
  gas::Animation animation = gas::Animation::model("Stand", 10.0f);
  animation.loop();
  Object object { Object::PLAYER, o, DOWN, std::move(animation) };
  return object;
}

Object newBoxObject(int x, int y)
{
  glhckObject* o = texturedCube(2 * GRID_SIZE / 5.0f, "model/box.png");
  glhckObjectPositionf(o, x * GRID_SIZE, 0, y * GRID_SIZE);
  Object object { Object::BOX, o, UP, gas::Animation::NONE };
  return object;
}

Coordinates findPlayer(Game* game)
{
  for(std::vector<Tile>& rows : game->level.tiles)
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
  return game->level.tiles.at(y).at(x);
}

gas::Animation pushAnimationBox(Direction direction)
{
  Coordinates& delta = DIRECTIONS[direction];

  return gas::Animation::sequential({
    gas::Animation::pause(0.1),
    gas::Animation::parallel({
     gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_X, gasEasingLinear, delta.x * GRID_SIZE, 0.7),
     gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_Z, gasEasingLinear, delta.y * GRID_SIZE, 0.7)
    }),
    gas::Animation::pause(0.1)
  });
}

gas::Animation pushAnimationPlayer(Direction direction, Direction facing)
{
  Coordinates& delta = DIRECTIONS[direction];
  int const directionAngle = DIRECTION_ANGLES[direction];
  int const facingAngle = DIRECTION_ANGLES[facing];
  int const rotation = directionAngle - facingAngle > 180
      ? (directionAngle - 360) - facingAngle
      : directionAngle - facingAngle;

  return gas::Animation::parallel({
    gas::Animation::sequential({
      gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_ROT_Y, gasEasingLinear, rotation, 0.1),
      gas::Animation::parallel({
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_X, gasEasingLinear, delta.x * GRID_SIZE/3, 0.1),
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_Z, gasEasingLinear, delta.y * GRID_SIZE/3, 0.1)
      }),
      gas::Animation::parallel({
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_X, gasEasingLinear, delta.x * GRID_SIZE, 0.7),
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_Z, gasEasingLinear, delta.y * GRID_SIZE, 0.7),
      }),
      gas::Animation::parallel({
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_X, gasEasingLinear, -delta.x * GRID_SIZE/3, 0.1),
       gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_Z, gasEasingLinear, -delta.y * GRID_SIZE/3, 0.1),
      }),
    }),
    gas::Animation::model("Push", 1.0)
  });
}

gas::Animation walkAnimationPlayer(Direction direction, Direction facing)
{
  Coordinates& delta = DIRECTIONS[direction];
  int const directionAngle = DIRECTION_ANGLES[direction];
  int const facingAngle = DIRECTION_ANGLES[facing];
  int const rotation = directionAngle - facingAngle > 180
      ? (directionAngle - 360) - facingAngle
      : directionAngle - facingAngle;

  return gas::Animation::parallel({
    gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_ROT_Y, gasEasingLinear, rotation, 0.1),
    gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_X, gasEasingLinear, delta.x * GRID_SIZE, 0.5),
    gas::Animation::delta(GAS_NUMBER_ANIMATION_TARGET_Z, gasEasingLinear, delta.y * GRID_SIZE, 0.5),
    gas::Animation::model("Run", 0.5)
  });
}

void animationComplete(glhckObject* object, void* userdata)
{
  Game* game = static_cast<Game*>(userdata);
  game->animating = false;
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

  bool pushing = false;

  if(destinationTile.object.type != Object::NONE)
  {
    Coordinates pushDestination = {destination.x + delta.x, destination.y + delta.y};
    Tile& pushDestinationTile = getTile(game, pushDestination.x, pushDestination.y);
    if(pushDestinationTile.type == Tile::WALL || pushDestinationTile.object.type != Object::NONE)
    {
      return;
    }

    pushing = true;
    destinationTile.object.a = pushAnimationBox(direction);
    pushDestinationTile.object = std::move(destinationTile.object);
    destinationTile.object = NO_OBJECT;
  }

  game->animating = true;

  currentTile.object.a = gas::Animation::sequential({
    pushing ? pushAnimationPlayer(direction, currentTile.object.facing) : walkAnimationPlayer(direction, currentTile.object.facing),
    gas::Animation::action(animationComplete, game),
    gas::Animation::model("Stand", 10.0f).loop()
  });

  currentTile.object.facing = direction;
  destinationTile.object = std::move(currentTile.object);
  currentTile.object = NO_OBJECT;
}

Tile createTile(LevelPack::Level::Tile const tile, int const x, int const y)
{
  switch(tile)
  {
    case LevelPack::Level::NONE: return newEmptyTile(x, y);
    case LevelPack::Level::FLOOR: return newFloorTile(x, y);
    case LevelPack::Level::WALL: return newWallTile(x, y);
    case LevelPack::Level::BOX: return newFloorTile(x, y, newBoxObject(x, y));
    case LevelPack::Level::TARGET: return newTargetTile(x, y);
    case LevelPack::Level::PLAYER: return newFloorTile(x, y, newPlayerObject(x, y));
    default: return newEmptyTile(x, y);
  }
}

void loadLevel(Game* game, LevelPack::Level const& level)
{
  for(auto& row : game->level.tiles)
  {
    for(Tile& tile : row)
    {
      if(tile.type != Tile::NONE)
      {
        glhckObjectFree(tile.o);
      }
      if(tile.object.type != Object::NONE)
      {
        glhckObjectFree(tile.object.o);
      }
      if(tile.object.a)
      {
        tile.object.a = gas::Animation::NONE;
      }
    }
  }

  game->level.width = level.width;
  game->level.height = level.height;
  game->level.name = level.name;

  for(auto levelRow : level.tiles)
  {
    int y = game->level.tiles.size();
    game->level.tiles.push_back({});
    auto& row = game->level.tiles.back();
    for(auto tile : levelRow)
    {
      int x = game->level.tiles.back().size();
      row.push_back(createTile(tile, x, y));
    }
  }

  game->animating = false;

  if(game->camera)
  {
    glhckCameraFree(game->camera);
  }
  game->camera = glhckCameraNew();

  glhckCameraProjection(game->camera, GLHCK_PROJECTION_PERSPECTIVE);
  glhckObjectPositionf(glhckCameraGetObject(game->camera),
                       game->level.width * GRID_SIZE / 2,
                       game->level.width * GRID_SIZE * 2,
                       (game->level.height * GRID_SIZE));
  glhckObjectTargetf(glhckCameraGetObject(game->camera),
                     game->level.width * GRID_SIZE / 2,
                     0,
                     game->level.height * GRID_SIZE / 2);

  glhckCameraRange(game->camera, 0.1f, 100.0f);
  glhckCameraFov(game->camera, 45);
  glhckCameraUpdate(game->camera);
}

Game* newGame(const LevelPack::Level& level)
{
  Game* game = new Game;
  game->camera = nullptr;

  loadLevel(game, level);

  return game;
}

void playGame(Game* game, glfwContext& ctx)
{
  if(!game->animating)
  {
    if(glfwGetKey(ctx.window, GLFW_KEY_ESCAPE))
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

  for(std::vector<Tile>& row : game->level.tiles)
  {
    for(Tile& tile : row)
    {
      if(tile.object.a)
      {
        tile.object.a.animate(tile.object.o, ctx.deltaTime);
        if(tile.object.a.getState() == GAS_ANIMATION_STATE_FINISHED)
        {
          tile.object.a = gas::Animation::NONE;
        }
      }
    }
  }

  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT | GLHCK_COLOR_BUFFER_BIT);
  glhckCameraUpdate(game->camera);

  for(std::vector<Tile>& rows : game->level.tiles)
  {
    for(Tile& tile : rows)
    {
      if(tile.type != Tile::NONE)
      {
        glhckObjectDraw(tile.o);
      }
      if(tile.object.type != Object::NONE)
      {
        glhckObjectDraw(tile.object.o);
      }
    }
  }
}

void endGame(Game* game)
{
  delete game;
}

bool levelFinished(Level const& level)
{
  for(auto& row : level.tiles)
  {
    for(Tile const& tile : row)
    {
      if(tile.type == Tile::TARGET && tile.object.type != Object::BOX)
      {
        return false;
      }
    }
  }

  return true;
}

bool gameFinished(Game* game)
{
  return !game->animating && levelFinished(game->level);
}
