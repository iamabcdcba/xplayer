#include "player.h"
#include "listener.h"
#include <stdio.h>
#include <iostream>

int main(int argc, char* argv[])
{
  char* fname;
  if (argc < 2) {
    fprintf(stderr, "Use format ./testapp <filename>\n");
    fname = "ueye.mp4";
  } else {
    fname = argv[1];
  }

  av_register_all();
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER)) {
    fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
    exit(1);
  }

  Listener listener;
  Player player(fname, &listener);

  // main loop
  SDL_Event event;
  while (1) {
    SDL_WaitEvent(&event);
    switch(event.type) {
    case SDL_KEYDOWN:
      if (event.key.keysym.sym == SDLK_ESCAPE) 
	player.stop();
      break;
    case SDL_MOUSEBUTTONDOWN:
      {
	if (event.button.button == SDL_BUTTON_LEFT)
	  player.play();
	else if (event.button.button == SDL_BUTTON_RIGHT)
	  player.pause();
      }
      break;
    case SDL_QUIT:      
      player.stop();
      SDL_Quit();
      return 0;
    case FF_REFRESH_EVENT:
      VideoPicture::update(event.user.data1);
      break;
    }
  }

  return 0;
}
