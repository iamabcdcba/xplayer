#include "packetqueue.h"
#include <iostream>

PacketQueue::PacketQueue(int pmax_size)
{
  max_size = pmax_size;
  stopped = paused = false;
  first = last = NULL;
  number = size = 0;
  mutex = SDL_CreateMutex();
  cond = SDL_CreateCond();
}

PacketQueue::~PacketQueue()
{
  clear();
}

bool PacketQueue::is_full()
{
  return size >= max_size;
}

void PacketQueue::stop()
{
  play();
  SDL_LockMutex(mutex);
  stopped = true;
  SDL_UnlockMutex(mutex);
}

void PacketQueue::clear()
{
  AVPacketList *pktl;
  while (first != NULL) {
    pktl = first;
    first = first->next;
    number--;
    size -= pktl->pkt.size;
    av_free(pktl);
  }
  last = NULL;
}

int PacketQueue::push(AVPacket *pkt)
{
  if (av_dup_packet(pkt) < 0)
    return -1;
  AVPacketList *pktl = static_cast<AVPacketList*>(av_malloc(sizeof(AVPacketList)));
  if (!pktl)
    return -1;
  pktl->pkt = *pkt;
  pktl->next = NULL;
  SDL_LockMutex(mutex);
  if (!last) 
    first = pktl;
  else
    last->next = pktl;
  last = pktl;
  number++;
  size += pktl->pkt.size;
  SDL_CondSignal(cond);
  SDL_UnlockMutex(mutex);
  return 0;
}

int PacketQueue::pop(AVPacket* pkt)
{
  AVPacketList* pktl;
  int ret = -1;
  SDL_LockMutex(mutex);
  while (!stopped) {
    if (!first || paused)
      SDL_CondWait(cond, mutex);
    else {
      pktl = first;
      first = first->next;
      if (!first) 
	last = NULL;
      number--;
      size -= pktl->pkt.size;
      *pkt = pktl->pkt;
      av_free(pktl);
      ret = 1;
      break;
    }
  }
  SDL_UnlockMutex(mutex);
  return ret;
}

void PacketQueue::pause()
{
  SDL_LockMutex(mutex);
  paused = true;
  SDL_UnlockMutex(mutex);
}

void PacketQueue::play()
{
  SDL_LockMutex(mutex);
  if (paused) {
    paused = false;
    SDL_CondSignal(cond);
  }
  SDL_UnlockMutex(mutex);
}

void PacketQueue::release()
{
  clear();
  stopped = false;
  first = last = NULL;
  number = size = 0;      
}
