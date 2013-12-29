#ifndef LEVELPACK_H
#define LEVELPACK_H

#include <string>
#include <vector>

class LevelPack
{
public:
  struct Level
  {
    enum Tile { NONE, FLOOR, WALL, BOX, TARGET, PLAYER };
    Level() : tiles(), width(0), height(0), name() {}
    std::vector<std::vector<Tile>> tiles;
    unsigned int width;
    unsigned int height;
    std::string name;
  };

  LevelPack(std::string const& filename);

  std::string const& getName() const;
  std::string const& getDescription() const;
  std::vector<Level> const& getLevels() const;
  Level const& getLevel(unsigned int const i) const;
  int size() const;

private:
  void parseFile(std::string const& filename);
  void loadLevel(std::istream& stream);

  std::string name;
  std::string description;
  std::vector<Level> levels;
};

#endif // LEVELPACK_H
