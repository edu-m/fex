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
#include "trie.h"
#include <ncurses.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define FEX_VERSION "1.2.1"
static int startx = 0, starty = 0;
static char **choices = NULL;
static int n_choices;
static bool show_hidden_files = false;
static void free_cbuf(void);
static void load_directory(const char *);
static WINDOW *recreate_menu_window(void);

static void handle_exit(int status) {
  curs_set(1);
  clear();
  refresh();
  endwin();
  free_cbuf();
  FILE *fptr;
  char dir[BUFSIZE];
  snprintf(dir, sizeof(dir), "%s/.fexlastdir", getenv("HOME"));
  fptr = fopen(dir, "w");
  if (!fptr) {
    perror("fopen");
    exit(EXIT_FAILURE);
  }
  char cwd[BUFSIZE];
  if (platform_current_directory(cwd, sizeof(cwd)) == 0)
    fprintf(fptr, "%s", cwd);
  exit(status);
}

static void sighandler(int signum) {
  if (signum == SIGINT)
    handle_exit(EXIT_SUCCESS);
}

static int n_digits(int n) {
  int ret = 1;
  do {
    n /= 10;
    ++ret;
  } while (n);
  return ret;
}

static inline int isdigit(int c) { return c >= '0' && c <= '9'; }

static int digits_only(const char *s) {
  while (*s)
    if (!isdigit(*s++))
      return 0;
  return 1;
}

static int cmp_choices(const void *a, const void *b) {
  const char *sa = *(const char *const *)a;
  const char *sb = *(const char *const *)b;

  if (strcmp(sa, "..") == 0)
    return -1;
  if (strcmp(sb, "..") == 0)
    return +1;
  return strcmp(sa, sb);
}

static void get_file_info(const char *pathname, char *result, size_t size) {
  if (size)
    result[0] = '\0';
  platform_describe_file(pathname, result, size);
}

static void free_cbuf(void) {
  for (int i = 0; i < n_choices; i++)
    free(choices[i]);
  free(choices);
  choices = NULL;
  n_choices = 0;
}

static WINDOW *recreate_menu_window(void) {
  initscr();
  clear();
  noecho();
  cbreak();
  WINDOW *menu_win = newwin(LINES, COLS, starty, startx);
  keypad(menu_win, TRUE);
  curs_set(0);
  return menu_win;
}

static void print_menu(WINDOW *menu_win, int highlight) {
  int x = 2, y = 2, maxy = getmaxy(menu_win), visible_count = maxy - 2, first;
  if (n_choices <= visible_count)
    first = 0;
  else {
    first = highlight - 1;
    if (first > n_choices - visible_count)
      first = n_choices - visible_count + 1;
  }
  werase(menu_win);
  for (int i = first; i < first + visible_count && i < n_choices; i++) {
    PlatformFileKind kind = platform_file_kind(choices[i]);
    if ((highlight - 1) == i)
      wattron(menu_win, A_REVERSE);

    switch (kind) {
    case PLATFORM_FILE_SYMLINK:
      mvwprintw(menu_win, y, x, "%d\t| {%s}", i, choices[i]);
      break;
    case PLATFORM_FILE_DIRECTORY:
      mvwprintw(menu_win, y, x, "%d\t| [%s]", i, choices[i]);
      break;
    case PLATFORM_FILE_CHAR_DEVICE:
      mvwprintw(menu_win, y, x, "%d\t| ..%s..", i, choices[i]);
      break;
    case PLATFORM_FILE_BLOCK_DEVICE:
      mvwprintw(menu_win, y, x, "%d\t| _%s_", i, choices[i]);
      break;
    default:
      mvwprintw(menu_win, y, x, "%d\t| %s", i, choices[i]);
      break;
    }

    if ((highlight - 1) == i)
      wattroff(menu_win, A_REVERSE);
    y++;
  }
  wrefresh(menu_win);
}

static void print_logo(WINDOW *menu_win) {
  const char *logo[] = {"      :::::::::: :::::::::: :::    :::",
                        "     :+:        :+:        :+:    :+: ",
                        "    +:+        +:+         +:+  +:+   ",
                        "   :#::+::#   +#++:++#     +#++:+     ",
                        "  +#+        +#+         +#+  +#+     ",
                        " #+#        #+#        #+#    #+#     ",
                        "###        ########## ###    ###      "};
  clear();
  int n_lines = sizeof(logo) / sizeof(logo[0]), start_y = LINES / 4;
  for (int i = 0; i < n_lines; i++) {
    int start_x = (COLS - (int)strlen(logo[i])) / 2;
    mvwprintw(menu_win, start_y + i, start_x, "%s", logo[i]);
  }
  int s = -6;
  refresh();
  wattron(menu_win, A_REVERSE);
  mvwprintw(menu_win, LINES + s--, 0,
            "This is free software, and you are welcome to redistribute it "
            "under certain conditions.\nThis program is distributed in the "
            "hope that it will be useful, but WITHOUT ANY WARRANTY;\nwithout "
            "even the implied warranty of MERCHANTABILITY or FITNESS FOR A "
            "PARTICULAR PURPOSE.\nSee the GNU General Public License for more "
            "details: <http://www.gnu.org/licenses/>");
  mvwprintw(menu_win, LINES + s--, 0,
            "This program comes with ABSOLUTELY NO WARRANTY;");
  mvwprintw(menu_win, LINES + s--, 0,
            "fex " FEX_VERSION " Copyright (C) 2025 Eduardo Meli");
  wattroff(menu_win, A_REVERSE);
  wgetch(menu_win);
}

static void handle_keyw(WINDOW *menu_win, int n_c, int *highlight) {
  mvprintw(LINES - 1, 0, ": ");
  int c = 0, count = 0, max_digits_val = n_digits(n_c), i = 0;
  char input_buffer[32] = {0};
  while (count < n_c && c != 27 && c != 261 && c != 108 &&
         n_digits(count) <= n_digits(n_c)) {
    clrtoeol();
    refresh();
    c = wgetch(menu_win);
    if ((c == KEY_BACKSPACE || c == 127) && i > 0) {
      i--;
      input_buffer[i] = '\0';
    } else if (c != 27 && c != 261 && c != 108 && c != 10) {
      if (i < 31) {
        input_buffer[i++] = (char)c;
        input_buffer[i] = '\0';
      }
    }
    mvprintw(LINES - 1, 0, ":%s", input_buffer);
    refresh();
    if (digits_only(input_buffer)) {
      count = atoi(input_buffer);
      if (count < n_c) {
        *highlight = count + 1;
        if (i == max_digits_val)
          break;
      } else
        *highlight = n_c + 1;
    }
    if (c == 10) {
      if (strncmp(input_buffer, "G", 2) == 0)
        *highlight = n_c + 1;
      else if (strncmp(input_buffer, "gg", 3) == 0)
        *highlight = 1;
      else if (strncmp(input_buffer, "vim", 3) == 0) {
        char *argv_local[] = {"vim", NULL, NULL};
        argv_local[1] =
            (input_buffer[3] == '!') ? "." : choices[*highlight - 1];
        endwin();
        if (platform_spawn_and_wait(argv_local) == -1)
          handle_exit(EXIT_FAILURE);
        menu_win = recreate_menu_window();
        load_directory(".");
        print_menu(menu_win, *highlight);
        refresh();
        break;
      } else if (strncmp(input_buffer, "w", 2) == 0) {
        print_logo(menu_win);
      }
      break;
    }
  }
  clrtoeol();
  refresh();
}

void load_directory(const char *dirpath) {
  if (platform_change_directory(dirpath) != 0) {
    perror("chdir");
    return;
  }
  free_cbuf();

  if (platform_list_directory(show_hidden_files, &choices, &n_choices) != 0) {
    perror("opendir");
    exit(EXIT_FAILURE);
  }
  // Sort with our custom comparator
  qsort(choices, n_choices, sizeof(*choices), cmp_choices);
}

static void error(const char *what) {
  fprintf(stderr, "%s\n", what);
  handle_exit(EXIT_FAILURE);
}

static void print_licensing(WINDOW *menu_win) {
  const char *t0 = "fex " FEX_VERSION " Copyright (C) 2025 Eduardo Meli";
  const char *t1 = "for copyright details type `:w`.";
  mvprintw(LINES - 4, COLS - (int)strlen(t0), "%s", t0);
  mvprintw(LINES - 3, COLS - (int)strlen(t1), "%s", t1);
  wrefresh(menu_win);
}

int main(int argc, char **argv) {
  signal(SIGINT, sighandler);
  WINDOW *menu_win;
  int highlight = 1, choice = 0, c;
  char info[BUFSIZE];
  if (argc < 2)
    load_directory(".");
  else {
    if (platform_is_directory(argv[1]))
      load_directory(argv[1]);
    else
      error("Cannot find selected directory");
  }
  initscr();
  clear();
  noecho();
  cbreak();
  startx = 0;
  starty = 1;
  menu_win = newwin(LINES, COLS, starty, startx);
  keypad(menu_win, TRUE);
  curs_set(0);
  refresh();
  print_menu(menu_win, highlight);
  print_licensing(menu_win);
  refresh();
  while (1) {
    get_file_info(choices[highlight - 1], info, sizeof(info));
    mvprintw(0, 0, "%s", info);
    clrtoeol();
    refresh();
    c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
    case 'k':
      highlight = (highlight == 1) ? n_choices : highlight - 1;
      break;
    case KEY_DOWN:
    case 'j':
      highlight = (highlight == n_choices) ? 1 : highlight + 1;
      break;
    case 260:
    case 'h':
      load_directory("..");
      highlight = 1;
      memset(info, 0, sizeof(info));
      break;
    case 261:
    case 'l':
    case 10: {
      if (platform_is_directory(choices[highlight - 1])) {
        load_directory(choices[highlight - 1]);
        highlight = 1;
        memset(info, 0, sizeof(info));
      } else {
        if (platform_is_text_file(choices[highlight - 1])) {
          char *argv_local[] = {"vim", choices[highlight - 1], NULL};
          endwin();
          if (platform_spawn_and_wait(argv_local) == -1)
            handle_exit(EXIT_FAILURE);
          menu_win = recreate_menu_window();
          print_menu(menu_win, highlight);
          refresh();
          break;
        }
        platform_open_path(choices[highlight - 1]);
      }
      break;
    }
    case 'q':
      choice = -1;
      break;
    case 'a':
      show_hidden_files = !show_hidden_files;
      load_directory(".");
      if (highlight > n_choices)
        highlight = n_choices;
      refresh();
      break;
    case ':':
      handle_keyw(menu_win, n_choices - 1, &highlight);
      break;
    case '/':
      handle_search(menu_win, &highlight, n_choices, choices);
      break;
    case '~':
      load_directory(getenv("HOME"));
      // if our current selection is further down than the current directory
      // allows we should move it up (could make sense to put it at the bottom,
      // or even in the middle?) might change this in the future to see what's
      // more convenient
      if (highlight > n_choices)
        highlight = 0;
      break;
    default:
      break;
    }
    print_menu(menu_win, highlight);
    if (choice)
      break;
  }
  handle_exit(EXIT_SUCCESS);
  return 0;
}
