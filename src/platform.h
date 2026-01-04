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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdbool.h>
#include <stddef.h>

typedef enum {
  PLATFORM_FILE_UNKNOWN = 0,
  PLATFORM_FILE_REGULAR,
  PLATFORM_FILE_DIRECTORY,
  PLATFORM_FILE_SYMLINK,
  PLATFORM_FILE_CHAR_DEVICE,
  PLATFORM_FILE_BLOCK_DEVICE,
} PlatformFileKind;

int platform_change_directory(const char *);
int platform_list_directory(bool, char ***, int *);
int platform_current_directory(char *, size_t);
PlatformFileKind platform_file_kind(const char *);
bool platform_is_directory(const char *);
int platform_is_text_file(const char *);
int platform_describe_file(const char *, char *, size_t);
int platform_spawn_and_wait(char *const[]);
int platform_open_path(const char *);

#endif
