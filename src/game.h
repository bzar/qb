#ifndef GAME_H
#define GAME_H

#include "glfwcontext.h"
#include <iostream>
#include <string>
#include <vector>

struct Game;
struct Level;

Level* loadLevel(std::istream& stream);
Level* loadLevel(std::vector<std::string> const& rows);
Level* loadLevel(std::string const& str);
void freeLevel(Level* level);

Game* newGame(Level* level);
void playGame(Game* game, glfwContext& ctx);
bool gameFinished(Game* game);
void endGame(Game* game);

#endif // GAME_H
