#pragma once
#include <stdio.h>

extern "C" {
  #include <libavutil/frame.h>
}

class Listener
{
 private:
  int number;
  
 public:
  Listener(): number(0)
  {
  }
  void initialized()
  {
    fprintf(stdout, "Initialization successfully finished\n");
  }
 
  void error(const char* errMsg)
  {
    fprintf(stderr, "Error: %s\n", errMsg);
  }
  
  void finished()
  {
    fprintf(stdout, "Playing finished\n");    
  }

  void frameAvailable(AVFrame* pFrame)
  {
    ; // fprintf(stderr, "Got frame number %d", number);
  }  
};
