#include <signal.h>

#include "pg_yregress.h"

static void signal_handler(int signum) { instances_cleanup(); }

void register_sighandler() {
  struct sigaction sa;
  sa.sa_handler = signal_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;

  sigaction(SIGINT, &sa, NULL);  // For Ctrl+C
  sigaction(SIGTERM, &sa, NULL); // For termination request
  sigaction(SIGABRT, &sa, NULL); // For abort
}
