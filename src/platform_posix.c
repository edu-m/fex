// Copyright (c) 2025 Eduardo Meli
/*
This file is part of fex.

fex is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

fex is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with fex.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "platform.h"
#include "xdg.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static void free_partial_names(char **names, int count) {
  for (int i = 0; i < count; i++)
    free(names[i]);
  free(names);
}

int platform_change_directory(const char *path) { return chdir(path); }

int platform_list_directory(bool show_hidden_files, char ***names,
                            int *count) {
  DIR *dir = opendir(".");
  if (!dir)
    return -1;

  *names = NULL;
  *count = 0;

  struct dirent *entry;
  while ((entry = readdir(dir)) != NULL) {
    if (strcmp(entry->d_name, ".") == 0)
      continue;
    if (!show_hidden_files && entry->d_name[0] == '.' && entry->d_name[1] != '.')
      continue;

    char **tmp = realloc(*names, (*count + 1) * sizeof(*tmp));
    if (!tmp) {
      free_partial_names(*names, *count);
      *names = NULL;
      *count = 0;
      closedir(dir);
      return -1;
    }
    *names = tmp;
    (*names)[*count] = strdup(entry->d_name);
    if (!(*names)[*count]) {
      free_partial_names(*names, *count);
      *names = NULL;
      *count = 0;
      closedir(dir);
      return -1;
    }
    (*count)++;
  }
  closedir(dir);
  return 0;
}

int platform_current_directory(char *buffer, size_t size) {
  return getcwd(buffer, size) ? 0 : -1;
}

PlatformFileKind platform_file_kind(const char *path) {
  struct stat st;
  if (lstat(path, &st) != 0)
    return PLATFORM_FILE_UNKNOWN;
  switch (st.st_mode & S_IFMT) {
  case S_IFLNK:
    return PLATFORM_FILE_SYMLINK;
  case S_IFDIR:
    return PLATFORM_FILE_DIRECTORY;
  case S_IFCHR:
    return PLATFORM_FILE_CHAR_DEVICE;
  case S_IFBLK:
    return PLATFORM_FILE_BLOCK_DEVICE;
  default:
    return PLATFORM_FILE_REGULAR;
  }
}

bool platform_is_directory(const char *path) {
  struct stat st;
  if (stat(path, &st) != 0)
    return false;
  return S_ISDIR(st.st_mode);
}

int platform_is_text_file(const char *filepath) {
  char command[512], mime[1024] = {0};
  FILE *fp;
  snprintf(command, sizeof(command), "file --mime-type -b \"%s\"", filepath);
  if (!(fp = popen(command, "r")))
    return 0;
  if (!fgets(mime, sizeof(mime), fp)) {
    pclose(fp);
    return 0;
  }
  pclose(fp);
  mime[strcspn(mime, "\n")] = '\0';
  return strncmp(mime, "text/", 5) == 0;
}

int platform_describe_file(const char *filepath, char *buffer, size_t size) {
  char command[512];
  FILE *fp;
  snprintf(command, sizeof(command), "file \"%s\"", filepath);
  fp = popen(command, "r");
  if (!fp)
    return -1;

  if (size)
    buffer[0] = '\0';

  char line[1024];
  while (fgets(line, sizeof(line), fp))
    strncat(buffer, line, size - strlen(buffer) - 1);

  return pclose(fp);
}

int platform_spawn_and_wait(char *const argv[]) {
  pid_t pid = fork();
  if (pid == 0) {
    execvp(argv[0], argv);
    _exit(EXIT_FAILURE);
  } else if (pid < 0) {
    return -1;
  }
  int status;
  if (waitpid(pid, &status, 0) == -1)
    return -1;
  return status;
}

int platform_open_path(const char *path) { return openFile(path); }
