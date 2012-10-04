#include <stdio.h>
#include <time.h>
#include "log.h"

void log_string(const char *str)
{
  FILE *f = fopen(LOGFILE, "a");
  if (f)
    {
      time_t timer;
      struct tm *timeinfo;
      char timestr[32];

      timer = time(NULL);
      timeinfo = localtime(&timer);
      strftime(timestr, sizeof(timestr), "%c", timeinfo);

      fprintf(f, "%s: %s\n", timestr, str);
      fclose(f);
    }
}

