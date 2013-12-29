#include "levelpack.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <queue>
#include <set>
#include <utility>

LevelPack::LevelPack(const std::string& filename) : name(), description(), levels()
{
  parseFile(filename);
}

std::string const& LevelPack::getName() const
{
  return name;
}

std::string const& LevelPack::getDescription() const
{
  return description;
}

std::vector<LevelPack::Level> const& LevelPack::getLevels() const
{
  return levels;
}

const LevelPack::Level& LevelPack::getLevel(const unsigned int i) const
{
  return levels.at(i);
}

int LevelPack::size() const
{
  return levels.size();
}

void LevelPack::parseFile(const std::string& filename)
{
  std::ifstream ifs(filename);
  std::string line;

  while(getline(ifs, line) && line.front() == ';')
  {
    name = line.size() >= 3 ? line.substr(2) : "<unnamed>";
  }

  std::ostringstream descriptionStream;
  while(getline(ifs, line) && !line.empty())
  {
    if(line.size() >= 3)
    {
      descriptionStream << line.substr(2);
    }
    descriptionStream << std::endl;
  }
  description = descriptionStream.str();

  while(ifs)
  {
    loadLevel(ifs);
  }
}

void LevelPack::loadLevel(std::istream& stream)
{
  levels.push_back(Level());
  Level& level = levels.back();

  std::string row;
  int playerX = -1;
  int playerY = -1;
  while(std::getline(stream, row))
  {
    if(row.empty())
      break;

    if(row.front() == ';')
    {
      level.name = row.size() >= 3 ? row.substr(2) : "<unnamed>";
    }
    else
    {
      int y = level.tiles.size();

      level.tiles.push_back({});
      std::vector<Level::Tile>& levelRow = level.tiles.back();
      for(char const& c : row)
      {
        int x = levelRow.size();
        if(c == '#')
        {
          levelRow.push_back(Level::WALL);
        }
        else if(c == ' ')
        {
          levelRow.push_back(Level::FLOOR);
        }
        else if(c == '.')
        {
          levelRow.push_back(Level::TARGET);
        }
        else if(c == '@' || c == '+')
        {
          playerY = y;
          playerX = x;
          levelRow.push_back(Level::PLAYER);
        }
        else if(c == '$')
        {
          levelRow.push_back(Level::BOX);
        }
      }

      if(levelRow.size() > level.width)
      {
        level.width = levelRow.size();
      }
    }
  }

  level.height = level.tiles.size();

  // Determine playable area
  std::queue<std::pair<int, int>> queue;
  queue.push(std::make_pair(playerX, playerY));
  std::set<std::pair<int, int>> playArea;
  while(!queue.empty())
  {
    std::pair<int, int> coords = queue.front();
    queue.pop();

    // If coordinates are inside level boundaries
    // and are not already processed
    // and contain traversable terrain
    // add coordinates to play area
    // and enqueue neighbors for processing
    if(coords.second < level.tiles.size()
       && coords.first < level.tiles.at(coords.second).size()
       && playArea.find(std::make_pair(coords.first, coords.second)) == playArea.end()
       && level.tiles.at(coords.second).at(coords.first) != Level::WALL)
    {
      playArea.insert(coords);
      queue.push(std::make_pair(coords.first - 1, coords.second));
      queue.push(std::make_pair(coords.first + 1, coords.second));
      queue.push(std::make_pair(coords.first, coords.second - 1));
      queue.push(std::make_pair(coords.first, coords.second + 1));
    }
  }

  // Make non-playable non-wall area empty
  for(int y = 0; y < level.tiles.size(); ++y)
  {
    for(int x = 0; x < level.tiles.at(y).size(); ++x)
    {
      Level::Tile& tile = level.tiles.at(y).at(x);
      if(tile != Level::WALL && playArea.find(std::make_pair(x, y)) == playArea.end())
      {
        tile = Level::NONE;
      }
    }
  }
}
