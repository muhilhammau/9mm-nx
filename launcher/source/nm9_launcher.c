/* nm9_launcher.c -- the launcher's check for the game's data folder: the game
 * reads its .gla assets from gameloft/games/Gloft9MHM next to the APK. MIT. */
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include "launcher.h"
#include "rt_settings.h"

#define DATA_DIR "sdmc:" PORT_ROOT_PATH "/gameloft/games/Gloft9MHM"

int port_launcher_check(char *status, size_t scap, char *help, size_t hcap) {
  struct stat st;
  if (stat(DATA_DIR, &st) == 0 && S_ISDIR(st.st_mode)) {
    snprintf(status, scap, "Data: gameloft/games/Gloft9MHM found");
    return 0;
  }
  snprintf(status, scap, "Data: MISSING");
  snprintf(help, hcap,
           "Copy the game's data folder (the .gla files, from\n"
           "/sdcard/gameloft/games/Gloft9MHM on your phone) to:\n  " PORT_ROOT_PATH
           "/gameloft/games/Gloft9MHM/");
  return 1;
}

void port_launcher_instructions(void) {
  printf("  1. put the APK of your own " PORT_APK_DESC "\n"
         "     (any file name) in " PORT_ROOT_PATH "\n"
         "  2. put the game's data folder in " PORT_ROOT_PATH
         "/gameloft/games/Gloft9MHM\n");
}
