#ifndef GAME_H
#define GAME_H

#include "glfwcontext.h"

struct Game;

Game* newGame();
void playGame(Game* game, glfwContext& ctx);
void endGame(Game* game);

#endif // GAME_H
