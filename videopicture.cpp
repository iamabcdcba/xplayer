#include "videopicture.h"
#include <iostream>

VideoPicture::VideoPicture(int x, int y, int w, int h, int psize)
{
  rindex = windex = number = 0;
  size = psize;
  queue = static_cast<VideoBitmap*>(av_malloc(sizeof(VideoBitmap) * size));
  memset(queue, 0, sizeof(VideoBitmap) * size);
  stopped = false;

  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
  screen_mutex = SDL_CreateMutex();
  set_video_context(NULL);

#ifndef __DARWIN__
  screen = SDL_SetVideoMode(w, h, x, y);
#else
  screen = SDL_SetVideoMode(w, h, x, y);
#endif
}

VideoPicture::~VideoPicture()
{
  av_free(queue);
  queue = NULL;
}

void VideoPicture::set_video_context(AVCodecContext *pvideo_ctx)
{
  video_ctx = pvideo_ctx;
  if (video_ctx)
    sws_ctx = sws_getContext(video_ctx->width, video_ctx->height,
			     video_ctx->pix_fmt, video_ctx->width,
			     video_ctx->height, AV_PIX_FMT_YUV420P,
			     SWS_BILINEAR, NULL, NULL, NULL);  
}

bool VideoPicture::is_null()
{
  return !screen;
}

void VideoPicture::alloc()
{
  if (is_null() || stopped)
    return;
  VideoBitmap* vb = &queue[windex];
  if(vb->bmp)
    SDL_FreeYUVOverlay(vb->bmp);
  SDL_LockMutex(screen_mutex);
  vb->bmp = SDL_CreateYUVOverlay(video_ctx->width,
				video_ctx->height,
				SDL_YV12_OVERLAY,
				screen);
  SDL_UnlockMutex(screen_mutex);
  vb->width = video_ctx->width;
  vb->height = video_ctx->height;
  vb->allocated = 1;
}

int VideoPicture::push(AVFrame *pFrame)
{
  SDL_LockMutex(mutex);
  while (number >= size && (!stopped))
    SDL_CondWait(cond, mutex);
  SDL_UnlockMutex(mutex);

  if (stopped)
    return -1;

  if (!video_ctx)
    return -2;

  VideoBitmap* vb = &queue[windex];
  if(!vb->bmp || vb->width != video_ctx->width || vb->height != video_ctx->height) {
    vb->allocated = 0;
    alloc();
  }
  
  if((vb->bmp) && (!stopped)) {
    if (!sws_ctx) {
      return -3;
    }

    SDL_LockYUVOverlay(vb->bmp);
    AVPicture pict;
    pict.data[0] = vb->bmp->pixels[0];
    pict.data[1] = vb->bmp->pixels[2];
    pict.data[2] = vb->bmp->pixels[1];

    pict.linesize[0] = vb->bmp->pitches[0];
    pict.linesize[1] = vb->bmp->pitches[2];
    pict.linesize[2] = vb->bmp->pitches[1];

    sws_scale(sws_ctx, (uint8_t const * const *)pFrame->data,
	      pFrame->linesize, 0, video_ctx->height, pict.data, pict.linesize);
    SDL_UnlockYUVOverlay(vb->bmp);

    if(++windex == size)
      windex = 0;

    SDL_LockMutex(mutex);
    number++;
    SDL_UnlockMutex(mutex);
  }
  return 0;
}

void VideoPicture::show()
{
  if (!video_ctx || is_null())
    return;
  
  SDL_Rect rect;
  float aspect_ratio = 0;
  int w, h, x, y;
  VideoBitmap* vb = &queue[rindex];
  if (vb->bmp) {
    if (video_ctx->sample_aspect_ratio.num)
      aspect_ratio = av_q2d(video_ctx->sample_aspect_ratio) *
	video_ctx->width / video_ctx->height;
    if (aspect_ratio <= 0.0)
      aspect_ratio = (float)video_ctx->width / (float)video_ctx->height;

    h = screen->h;
    w = ((int)rint(h * aspect_ratio)) & -3;
    if (w > screen->w) {
      w = screen->w;
      h = ((int)rint(w / aspect_ratio)) & -3;
    }
    
    x = (screen->w - w) / 2;
    y = (screen->h - h) / 2;
    
    rect.x = x;
    rect.y = y;
    rect.w = w;
    rect.h = h;

    SDL_LockMutex(screen_mutex);
    SDL_DisplayYUVOverlay(vb->bmp, &rect);
    SDL_UnlockMutex(screen_mutex);
  }
}

void VideoPicture::update()
{
  if (number == 0) {
    SDL_LockMutex(mutex);
    if (!stopped) {
      schedule_refresh(this, 1);
    }
    SDL_UnlockMutex(mutex);
  }
  else {
    schedule_refresh(this, 80);
    show();
    SDL_LockMutex(mutex);
    if (++rindex == size)
      rindex = 0;
    number--;
    SDL_CondSignal(cond);
    SDL_UnlockMutex(mutex);
  }
}

void VideoPicture::update(void* data)
{
  VideoPicture* vp = static_cast<VideoPicture*>(data);
  vp->update();
}

void VideoPicture::schedule_refresh(VideoPicture* vp, int delay)
{
  SDL_AddTimer(delay, sdl_refresh_timer_cb, vp);
}

Uint32 VideoPicture::sdl_refresh_timer_cb(Uint32 interval, void* data)
{
  SDL_Event event;
  event.type = FF_REFRESH_EVENT;
  event.user.data1 = data;
  SDL_PushEvent(&event);
  return 0;
}

void VideoPicture::play()
{
  stopped = false;
  schedule_refresh(this, 40);  
}

void VideoPicture::stop()
{
  SDL_LockMutex(mutex);
  stopped = true;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
}

void VideoPicture::release()
{
  for(int i = rindex; i != windex; i = (i + 1) % size)
    SDL_FreeYUVOverlay(queue[i].bmp);
  memset(queue, 0, sizeof(VideoBitmap) * size);  
  rindex = windex = number = 0;

  SDL_LockMutex(screen_mutex);
  SDL_FillRect(screen, NULL, SDL_MapRGB(screen->format, 0, 0, 0));
  SDL_Flip(screen);   
  SDL_UnlockMutex(screen_mutex);  
}
