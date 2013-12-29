#ifndef GAME_H
#define GAME_H

#include "glfwcontext.h"
#include "levelpack.h"
#include <iostream>
#include <vector>
#include <string>

struct Game;

Game* newGame(LevelPack::Level const& level);
void playGame(Game* game, glfwContext& ctx);
bool gameFinished(Game* game);
void endGame(Game* game);

#endif // GAME_H
