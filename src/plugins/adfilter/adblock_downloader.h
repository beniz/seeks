#ifndef ADBLOCK_DOWNLOADER_H
#define ADBLOCK_DOWNLOADER_H

#include <signal.h>
#include <string>

class adblock_downloader
{
  public:
    adblock_downloader();
    ~adblock_downloader();
    void start_timer();                                 // Start the timer loop
    void stop_timer();                                  // Stop the timer loop
  private:
    bool _timer_running;                                // True if the timer runs
    static void tick(int sig, siginfo_t *si, void *uc); // Static function for timer ticks
    //int download_lists();
    //bool download_list();
};

#endif
