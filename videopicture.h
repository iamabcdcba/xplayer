#pragma once
#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

extern "C" {
  #include <libavutil/frame.h>
  #include <libswscale/swscale.h>
  #include <libavcodec/avcodec.h>
  #include <libavutil/pixfmt.h>
}

#define FF_REFRESH_EVENT (SDL_USEREVENT)

class VideoPicture {
 public:
  VideoPicture(int x, int y, int w, int h, int size);
  ~VideoPicture();
  bool is_null();
  void set_video_context(AVCodecContext *video_ctx);
  void alloc();
  int push(AVFrame *pFrame);
  void play();
  void stop();
  void release();
  
  static void update(void* data);
  static Uint32 sdl_refresh_timer_cb(Uint32 delay, void* data);    

 private:
  typedef struct VideoBitmap {
    SDL_Overlay *bmp;
    int width, height;
    int allocated;
  } VideoBitmap;

  VideoBitmap* queue;
  int size;
  int number;
  int rindex;
  int windex;
  
  SDL_Surface *screen;
  SDL_mutex *screen_mutex;
  SDL_mutex *mutex;
  SDL_cond *cond;
  
  AVCodecContext *video_ctx;  
  struct SwsContext *sws_ctx;

  bool stopped;

  void show();
  void update();

  static void schedule_refresh(VideoPicture* vp, int delay);
};
