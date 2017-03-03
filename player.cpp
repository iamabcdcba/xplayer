#include "player.h"
#include <iostream>

Player::Player(const char* ppath, Listener* plistener)
{
  av_strlcpy(path, ppath, MAX_FILEPATH_LENGTH);
  listener = plistener;
  video_tid = decode_tid = init_tid = NULL;
  paused = false;
  stopped = true;
  eof = false;
  mutex = SDL_CreateMutex();
  pCurFrame = av_frame_alloc();
  pFrame = av_frame_alloc();
  
  queue = new PacketQueue(5 * 256 * 1024);  
  picture = new VideoPicture(0, 0, 640, 480, 1);
  
  if (picture->is_null()) {
    listener->error("SDL: Could not set video mode");
  }
}

Player::~Player()
{
  stop();
  delete queue;
  queue = NULL;
  delete picture;
  picture = NULL;
  av_frame_free(&pFrame);
  av_frame_free(&pCurFrame);
}

void Player::play()
{
  if (paused) {
    paused = false;
    queue->play();
  }
  if (stopped) {
    picture->play();
    eof = false;
    stopped = false;
    init_tid = SDL_CreateThread(init_thread, this);
    video_tid = SDL_CreateThread(video_thread, this);    
  }
}

void Player::pause()
{
  if (!paused) {
    paused = true;
    queue->pause();
  }
}

void Player::stop()
{
  if (!stopped) {
    play();
    stopped = true;
    queue->stop();
    picture->stop();
    int ret;
    if (init_tid)
      SDL_WaitThread(init_tid, &ret);
    if (video_tid)
      SDL_WaitThread(video_tid, &ret);
    if (decode_tid)
      SDL_WaitThread(decode_tid, &ret);
    video_tid = decode_tid = init_tid = NULL;
    queue->release();
    picture->release();
  }
}

AVFrame* Player::getFrame()
{
  SDL_LockMutex(mutex);
  if (pFrame)
    av_frame_copy(pCurFrame, pFrame);
  SDL_UnlockMutex(mutex);
  return pCurFrame;
}

int Player::init_thread(void* object)
{
  Player *player = static_cast<Player*>(object);
  player->init();
  return 0;
}

void Player::init()
{
  AVFormatContext *pFormatCtx;
  if (avformat_open_input(&pFormatCtx, path, NULL, NULL) != 0) {
    listener->error("FFMpeg: Could not open input format context");
    return;
  }
  format_ctx = pFormatCtx;
  if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
    listener->error("FFMpeg: Could not find stream info");
    return;
  }
  av_dump_format(pFormatCtx, 0, path, 0);
  video_index = -1;
  for(int i = 0; (i<pFormatCtx->nb_streams) && (video_index == -1); i++)
    if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO)
      video_index = i;
  if (video_index == -1) {
    listener->error("FFMpeg: Could not find video stream");
    return;
  }
  AVCodec* codec = avcodec_find_decoder(pFormatCtx->streams[video_index]->codec->codec_id);
  if (!codec) {
    listener->error("FFMpeg: Could not find video decoder");
    return;
  }
  AVCodecContext *codecCtx = avcodec_alloc_context3(codec);
  if (avcodec_copy_context(codecCtx, pFormatCtx->streams[video_index]->codec) != 0) {
    listener->error("FFMpeg: Could not copy codec context");
    return;
  }
  if (avcodec_open2(codecCtx, codec, NULL) < 0) {
    listener->error("FFMpeg: Could not open codec");
    return;
  }
  video_ctx = codecCtx;
  picture->set_video_context(codecCtx);
  decode_tid = SDL_CreateThread(decode_thread, this);
  listener->initialized();
}

int Player::decode_thread(void* object)
{
  Player *player = static_cast<Player*>(object);
  player->decode();
  return 0;
}

void Player::decode()
{
  int err_code;
  char err_msg[512];
  AVPacket packet;

  while (1) {
    if (stopped)
      break;    
    if(queue->is_full()) {
      SDL_Delay(10);
      continue;
    }
    if (av_read_frame(format_ctx, &packet) < 0) {
      eof = true;
      break;
      if (format_ctx->pb->error == 0) {	
	SDL_Delay(100);
	continue;
      } else
	break;      
    } 
    if (packet.stream_index == video_index) 
      queue->push(&packet);
    else
      av_free_packet(&packet);
  }
  while (!stopped) {
    SDL_Delay(100);    
  }
  queue->release();  
  avcodec_close(video_ctx);
  avformat_close_input(&format_ctx);
  listener->finished();  
}

int Player::video_thread(void* object)
{
  Player *player = static_cast<Player*>(object);
  player->render();
  return 0;  
}

void Player::render()
{
  int frameFinished;
  AVPacket pkt1, *packet = &pkt1;
  while (1) {
    if (stopped)
      break;
    if (queue->pop(packet, eof) < 0) {
      if (eof) {
	picture->stop();
	break;
      }
    }
    SDL_LockMutex(mutex);
    avcodec_decode_video2(video_ctx, pFrame, &frameFinished, packet);
    listener->frameAvailable(pFrame);
    SDL_UnlockMutex(mutex);
    if (frameFinished && (picture->push(pFrame) < 0))
      break;
    av_free_packet(packet);
  }
  picture->release();
  stopped = true;  
}
