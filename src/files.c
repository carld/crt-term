
#include <dirent.h>
#include <stdio.h>
#include <libgen.h>
#include <string.h>
#include <stdlib.h>

/* look for filename in paths */
char * find_file(const char *filename, const char *paths[], int pathc)
{
  char *found_path = NULL;
  int i;

  for (i = 0; i < pathc; i++) 
  {
    struct dirent **namelist;
    int n;

    if (paths[i] == NULL) continue;

    n = scandir(paths[i], &namelist, NULL, alphasort);

    if (n < 0) {
      printf("%s\n",paths[i]);
      perror("scandir");
    } else {
      while (n--) {
        if (strcmp(namelist[n]->d_name, filename)==0) {
          int len = strlen(filename) + strlen(paths[i]) + 1;

          found_path = calloc(len, sizeof (char));

          snprintf(found_path, len+1, "%s/%s", paths[i], namelist[n]->d_name);
        }
        free(namelist[n]);
      }
      free(namelist);
    }
  }

  return found_path;
}

