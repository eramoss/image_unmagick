#include <ctype.h>
#include <stdio.h> // rl needs FILE
#include <string.h>
#include <time.h>

#include <readline/history.h>
#include <readline/readline.h>

// fancy to print inside threads and dont mess up with rl
#define THREAD_PRINT(fmt, ...)                                                 \
  do {                                                                         \
    rl_save_prompt();                                                          \
    rl_replace_line("", 0);                                                    \
    rl_redisplay();                                                            \
    printf(fmt, ##__VA_ARGS__);                                                \
    fflush(stdout);                                                            \
    rl_restore_prompt();                                                       \
    rl_redisplay();                                                            \
  } while (0)

static inline char *trim(char *str) {
  char *end;

  while (*str && isspace((unsigned char)*str))
    str++;
  if (*str == 0)
    return str;

  end = str + strlen(str) - 1;
  while (end > str && isspace((unsigned char)*end))
    end--;
  *(end + 1) = '\0';

  return str;
}

static inline double now_ms() {
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}
