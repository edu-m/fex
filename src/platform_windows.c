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
#include <direct.h>
#include <process.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

static void free_partial_names(char **names, int count) {
  for (int i = 0; i < count; i++)
    free(names[i]);
  free(names);
}

int platform_change_directory(const char *path) { return _chdir(path); }

int platform_list_directory(bool show_hidden_files, char ***names, int *count) {
  WIN32_FIND_DATAA ffd;
  HANDLE hFind = FindFirstFileA("*", &ffd);
  if (hFind == INVALID_HANDLE_VALUE)
    return -1;

  *names = NULL;
  *count = 0;

  do {
    const char *name = ffd.cFileName;
    if (strcmp(name, ".") == 0)
      continue;
    if (!show_hidden_files && (ffd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) &&
        strcmp(name, "..") != 0)
      continue;

    char **tmp = realloc(*names, (*count + 1) * sizeof(*tmp));
    if (!tmp) {
      free_partial_names(*names, *count);
      *names = NULL;
      *count = 0;
      FindClose(hFind);
      return -1;
    }
    *names = tmp;
    (*names)[*count] = _strdup(name);
    if (!(*names)[*count]) {
      free_partial_names(*names, *count);
      *names = NULL;
      *count = 0;
      FindClose(hFind);
      return -1;
    }
    (*count)++;
  } while (FindNextFileA(hFind, &ffd) != 0);

  FindClose(hFind);
  return 0;
}

int platform_current_directory(char *buffer, size_t size) {
  return _getcwd(buffer, (int)size) ? 0 : -1;
}

PlatformFileKind platform_file_kind(const char *path) {
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES)
    return PLATFORM_FILE_UNKNOWN;
  if (attr & FILE_ATTRIBUTE_REPARSE_POINT)
    return PLATFORM_FILE_SYMLINK;
  if (attr & FILE_ATTRIBUTE_DIRECTORY)
    return PLATFORM_FILE_DIRECTORY;
  if (attr & FILE_ATTRIBUTE_DEVICE)
    return PLATFORM_FILE_CHAR_DEVICE;
  return PLATFORM_FILE_REGULAR;
}

bool platform_is_directory(const char *path) {
  DWORD attr = GetFileAttributesA(path);
  if (attr == INVALID_FILE_ATTRIBUTES)
    return false;
  return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

int platform_is_text_file(const char *filepath) {
  FILE *fp = fopen(filepath, "rb");
  if (!fp)
    return 0;
  unsigned char buf[1024];
  size_t n = fread(buf, 1, sizeof(buf), fp);
  fclose(fp);
  for (size_t i = 0; i < n; i++)
    if (buf[i] == '\0')
      return 0;
  return 1;
}

int platform_describe_file(const char *filepath, char *buffer, size_t size) {
  WIN32_FIND_DATAA data;
  HANDLE hFind = FindFirstFileA(filepath, &data);
  if (hFind == INVALID_HANDLE_VALUE) {
    if (size)
      snprintf(buffer, size, "Unknown");
    return -1;
  }
  FindClose(hFind);
  const char *kind = "file";
  if (data.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
    kind = "symlink";
  else if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
    kind = "directory";
  if (size)
    snprintf(buffer, size, "%s: %s (size: %lu bytes)", filepath, kind,
             (unsigned long)((((unsigned long long)data.nFileSizeHigh) << 32) |
                             data.nFileSizeLow));
  return 0;
}

int platform_spawn_and_wait(char *const argv[]) {
  return (int)_spawnvp(_P_WAIT, argv[0], (const char *const *)argv);
}

int platform_open_path(const char *path) {
  HINSTANCE res = ShellExecuteA(NULL, "open", path, NULL, NULL, SW_SHOWNORMAL);
  return ((INT_PTR)res <= 32) ? 1 : 0;
}
