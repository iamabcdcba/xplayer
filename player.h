#pragma once
#include "listener.h"
#include "videopicture.h"
#include "packetqueue.h"

extern "C" {
  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/pixfmt.h>
  #include <libavutil/avstring.h>
}

#include <SDL/SDL_thread.h>
#define MAX_FILEPATH_LENGTH 1024

class Player
{
 public:
  Player(const char* ppath, Listener* plistener);
  ~Player();  
  void play();
  void pause();
  void stop();
  AVFrame* getFrame();  
  
 private:
  char path[MAX_FILEPATH_LENGTH];
  Listener* listener;
  SDL_mutex* mutex;
  
  AVFormatContext *format_ctx;
  AVFrame* pFrame;
  AVFrame* pCurFrame;
  AVCodecContext *video_ctx;
  int video_index;
  
  PacketQueue* queue;
  VideoPicture* picture;

  bool stopped;
  bool paused;

  SDL_Thread      *init_tid;
  SDL_Thread      *decode_tid;
  SDL_Thread      *video_tid;

 private: 
  static int init_thread(void *object);
  static int decode_thread(void *object);
  static int video_thread(void *object);

  void init();
  void decode();
  void render();
};

