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
  gasAnimation* a;
};

Object const NO_OBJECT = { Object::NONE, nullptr, UP, nullptr };

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
  Level* level;
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

Tile newFloorTile(int x, int y)
{
  glhckObject* o = texturedCube(GRID_SIZE / 2.0f, "model/floor.jpg");
  glhckObjectPositionf(o, x * GRID_SIZE, -GRID_SIZE, y * GRID_SIZE);
  float brightness = (x + y) % 2 ? 224 : 255;
  glhckMaterialDiffuseb(glhckObjectGetMaterial(o), brightness, brightness, brightness, 255);
  Tile tile { Tile::FLOOR, NO_OBJECT, {x, y}, o };
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
  gasAnimation* animation = gas::animationLoop(gas::modelAnimationNew("Stand", 10.0f));
  Object object { Object::PLAYER, o, DOWN, animation };
  return object;
}

Object newBoxObject(int x, int y)
{
  glhckObject* o = texturedCube(2 * GRID_SIZE / 5.0f, "model/box.png");
  glhckObjectPositionf(o, x * GRID_SIZE, 0, y * GRID_SIZE);
  Object object { Object::BOX, o, UP, nullptr };
  return object;
}

Coordinates findPlayer(Game* game)
{
  for(std::vector<Tile>& rows : game->level->tiles)
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
  return game->level->tiles.at(y).at(x);
}

gasAnimation* pushAnimationBox(Direction direction)
{
  Coordinates& delta = DIRECTIONS[direction];

  return gas::sequentialAnimationNew({
    gas::pauseAnimationNew(0.1),
    gas::parallelAnimationNew({
     gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, delta.x * GRID_SIZE, 0.7),
     gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, delta.y * GRID_SIZE, 0.7)
    }),
    gas::pauseAnimationNew(0.1)
  });
}

gasAnimation* pushAnimationPlayer(Direction direction, Direction facing)
{
  Coordinates& delta = DIRECTIONS[direction];
  int const directionAngle = DIRECTION_ANGLES[direction];
  int const facingAngle = DIRECTION_ANGLES[facing];
  int const rotation = directionAngle - facingAngle > 180
      ? (directionAngle - 360) - facingAngle
      : directionAngle - facingAngle;

  return gas::parallelAnimationNew({
    gas::sequentialAnimationNew({
      gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_ROT_Y, GAS_EASING_LINEAR, rotation, 0.1),
      gas::parallelAnimationNew({
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, delta.x * GRID_SIZE/3, 0.1),
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, delta.y * GRID_SIZE/3, 0.1)
      }),
      gas::parallelAnimationNew({
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, delta.x * GRID_SIZE, 0.7),
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, delta.y * GRID_SIZE, 0.7),
      }),
      gas::parallelAnimationNew({
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, -delta.x * GRID_SIZE/3, 0.1),
       gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, -delta.y * GRID_SIZE/3, 0.1),
      }),
    }),
    gas::modelAnimationNew("Push", 1.0)
  });
}

gasAnimation* walkAnimationPlayer(Direction direction, Direction facing)
{
  Coordinates& delta = DIRECTIONS[direction];
  int const directionAngle = DIRECTION_ANGLES[direction];
  int const facingAngle = DIRECTION_ANGLES[facing];
  int const rotation = directionAngle - facingAngle > 180
      ? (directionAngle - 360) - facingAngle
      : directionAngle - facingAngle;

  return gas::parallelAnimationNew({
    gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_ROT_Y, GAS_EASING_LINEAR, rotation, 0.1),
    gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_X, GAS_EASING_LINEAR, delta.x * GRID_SIZE, 0.5),
    gas::numberAnimationNewDelta(GAS_NUMBER_ANIMATION_TARGET_Z, GAS_EASING_LINEAR, delta.y * GRID_SIZE, 0.5),
    gas::modelAnimationNew("Run", 0.5)
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
    pushDestinationTile.object = destinationTile.object;
    destinationTile.object = NO_OBJECT;
  }

  game->animating = true;
  if(currentTile.object.a)
  {
    gas::animationFree(currentTile.object.a);
  }

  currentTile.object.a = gas::sequentialAnimationNew({
    pushing ? pushAnimationPlayer(direction, currentTile.object.facing) : walkAnimationPlayer(direction, currentTile.object.facing),
    gas::actionNew(animationComplete, nullptr, nullptr, nullptr, game),
    gas::animationLoop(gas::modelAnimationNew("Stand", 10.0f))
  });

  currentTile.object.facing = direction;
  destinationTile.object = currentTile.object;
  currentTile.object = NO_OBJECT;
}

void loadLevel(Game* game, Level* level)
{
  game->animating = false;
  game->level = level;

  if(game->camera)
  {
    glhckCameraFree(game->camera);
  }
  game->camera = glhckCameraNew();

  glhckCameraProjection(game->camera, GLHCK_PROJECTION_PERSPECTIVE);
  glhckObjectPositionf(glhckCameraGetObject(game->camera),
                       game->level->width * GRID_SIZE / 2,
                       game->level->width * GRID_SIZE * 2,
                       (game->level->height * GRID_SIZE));
  glhckObjectTargetf(glhckCameraGetObject(game->camera),
                     game->level->width * GRID_SIZE / 2,
                     0,
                     game->level->height * GRID_SIZE / 2);

  glhckCameraRange(game->camera, 0.1f, 100.0f);
  glhckCameraFov(game->camera, 45);
  glhckCameraUpdate(game->camera);
}

Game* newGame(Level* level)
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

  for(std::vector<Tile>& row : game->level->tiles)
  {
    for(Tile& tile : row)
    {
      if(tile.object.a != nullptr)
      {
        gasAnimate(tile.object.a, tile.object.o, ctx.deltaTime);
        if(gasAnimationGetState(tile.object.a) == GAS_ANIMATION_STATE_FINISHED)
        {
          gasAnimationFree(tile.object.a);
          tile.object.a = nullptr;
        }
      }
    }
  }

  glhckRenderClear(GLHCK_DEPTH_BUFFER_BIT | GLHCK_COLOR_BUFFER_BIT);
  glhckCameraUpdate(game->camera);

  for(std::vector<Tile>& rows : game->level->tiles)
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


Level* loadLevel(std::istream& stream)
{
  std::vector<std::string> rows;
  std::string line;

  while(std::getline(stream, line))
  {
    if(line.empty())
      break;

    rows.push_back(line);
  }

  return loadLevel(rows);
}


Level* loadLevel(std::string const& str)
{
  std::istringstream iss(str);
  std::vector<std::string> rows;
  std::string line;

  while(std::getline(iss, line))
  {
    rows.push_back(line);
  }

  return loadLevel(rows);
}

Level* loadLevel(std::vector<std::string> const& rows)
{
  Level* level = new Level;
  level->height = rows.size();
  level->width = 0;

  for(auto i = rows.begin(); i != rows.end(); ++i)
  {
    std::string const& row = *i;

    if(row.front() == ';')
    {
      level->name = row.size() >= 3 ? row.substr(2) : "<unnamed>";
    }
    else
    {
      bool firstWallEncountered = false;
      int y = level->tiles.size();
      level->tiles.push_back({});
      std::vector<Tile>& levelRow = level->tiles.back();
      for(char const& c : row)
      {
        int x = levelRow.size();
        if(c == '#')
        {
          levelRow.push_back(newWallTile(x, y));
          firstWallEncountered = true;
        }
        else if(c == ' ')
        {
          levelRow.push_back(firstWallEncountered ? newFloorTile(x, y) : newEmptyTile(x, y));
        }
        else if(c == '.')
        {
          levelRow.push_back(newTargetTile(x, y));
        }
        else if(c == '@' || c == '+')
        {
          Tile tile = newFloorTile(x, y);
          tile.object = newPlayerObject(x, y);
          levelRow.push_back(tile);
        }
        else if(c == '$')
        {
          Tile tile = newFloorTile(x, y);
          tile.object = newBoxObject(x, y);
          levelRow.push_back(tile);
        }
      }

      if(levelRow.size() > level->width)
      {
        level->width = levelRow.size();
      }
    }
  }

  return level;
}


void freeLevel(Level* level)
{
  for(auto& row : level->tiles)
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
        gasAnimationFree(tile.object.a);
      }
    }
  }

  delete level;
}

bool levelFinished(Level* level)
{
  for(std::vector<Tile>& rows : level->tiles)
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

bool gameFinished(Game* game)
{
  return !game->animating && levelFinished(game->level);
}
