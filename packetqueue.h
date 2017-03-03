#include <SDL/SDL_thread.h>

extern "C" {
  #include <libavformat/avformat.h>
}

class PacketQueue {
 private:  
  AVPacketList* first;
  AVPacketList* last;
  int number;
  int max_size;
  int size;
  SDL_mutex *mutex;
  SDL_cond *cond;
  bool stopped;
  bool paused;

 public:
  PacketQueue(int max_size);
  ~PacketQueue();
  void play();
  void pause();
  void stop();
  void clear();
  int push(AVPacket *pkt);
  int pop(AVPacket *pkt, bool eof);
  void release();
  bool is_full();
};

