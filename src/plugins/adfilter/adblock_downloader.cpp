#include "adblock_downloader.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>

#include <iostream>
#include <string>

#define CLOCKID CLOCK_REALTIME
#define SIG SIGRTMIN

/*
 * Constructor
 * ----------------
 */
adblock_downloader::adblock_downloader()
{
  timer_t timerid;
  struct sigaction sa;
  struct sigevent sev;
  struct itimerspec its;

  // Timer creation (SIG is SIGRTMIN)
  sev.sigev_notify = SIGEV_SIGNAL;
  sev.sigev_signo = SIG;
  sev.sigev_value.sival_ptr = this;
  timer_create(CLOCKID, &sev, &timerid);

  // Timer frequency definition
  its.it_value.tv_sec = 1;
  its.it_value.tv_nsec = 0;
  its.it_interval.tv_sec = its.it_value.tv_sec;
  its.it_interval.tv_nsec = its.it_value.tv_nsec;
  timer_settime(timerid, 0, &its, NULL);

  // Signal handler on timer expiration
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = adblock_downloader::tick;
  sigemptyset(&sa.sa_mask);
  sigaction(SIG, &sa, NULL);

  this->_timer_running = false;
}

adblock_downloader::~adblock_downloader()
{
  this->stop_timer();
}

// FIXME Timer commet
void adblock_downloader::start_timer()
{
  sigset_t mask;

  // SIG set initialization
  sigemptyset(&mask);
  // Add SIG signal to the set
  sigaddset(&mask, SIG);
  // Add signal to the mask and unblock it
  sigprocmask(SIG_SETMASK, &mask, NULL);
  sigprocmask(SIG_UNBLOCK, &mask, NULL);
  this->_timer_running = true;
}

void adblock_downloader::stop_timer()
{
  struct sigaction sa;
  // Signal handler on timer expiration
  sa.sa_flags = SA_SIGINFO;
  sa.sa_sigaction = NULL;
  sigemptyset(&sa.sa_mask);
  sigaction(SIG, &sa, NULL);
  this->_timer_running = false;
}

void adblock_downloader::tick(int sig, siginfo_t *si, void *uc)
{
  // Pointer to this instanciated class cast
  adblock_downloader* c = reinterpret_cast<adblock_downloader*>(si->si_value.sival_ptr);

  // It the timer is running, let's go for the next iteration
  if(c->_timer_running == true) c->start_timer();
}
