#ifndef ADBLOCK_DOWNLOADER_H
#define ADBLOCK_DOWNLOADER_H

#include "adfilter.h"
#include "seeks_proxy.h"

#include <signal.h>
#include <string>
#include <curl/curl.h>

using namespace sp;
using namespace seeks_plugins;

class adblock_downloader
{
  public:
    adblock_downloader(adfilter* parent, std::string filename);
    ~adblock_downloader();
    void start_timer();                                 // Start the timer loop
    void stop_timer();                                  // Stop the timer loop
  private:
    bool _timer_running;                                // True if the timer runs
    static void tick(int sig, siginfo_t *si, void *uc); // Static function for timer ticks
    int download_lists();                               // Download all lists
    std::string _listfilename;                          // Local lists file path
    sp_mutex_t _curl_mutex;                             // Mutex thing for curl
    adfilter* parent;                                   // Parent (sp::plugin)
    static int _curl_writecb(char *data, size_t size, size_t nmemb, std::string *buffer);
    std::string _curl_buffer;
    char _curl_errorBuffer[CURL_ERROR_SIZE];
};

#endif
