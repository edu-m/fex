// fex, the minimalistic file explorer
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
#include "xdg.h"
#include <ctype.h>
#include <dirent.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int startx = 0;
int starty = 0;

char **choices = NULL;
int n_choices = 0;
bool hide_hidden_files = true;

int max(int a, int b) { return a > b ? a : b; }

int n_digits(int n) {
  int ret = 1;
  do {
    n /= 10;
    ++ret;
  } while (n != 0);
  return ret;
}

int is_text_file(const char *filepath) {
  char command[512];
  char mime[256] = {0};
  FILE *fp;

  snprintf(command, sizeof(command), "file --mime-type -b \"%s\"", filepath);
  fp = popen(command, "r");
  if (!fp) {
    perror("popen");
    return 0;
  }

  if (fgets(mime, sizeof(mime), fp) == NULL) {
    pclose(fp);
    return 0;
  }
  pclose(fp);
  mime[strcspn(mime, "\n")] = '\0';
  return (strncmp(mime, "text/", 5) == 0);
}

void get_file_info(const char *pathname, char *result, size_t result_size) {
  char command[512];
  FILE *fp;
  char buffer[256];

  snprintf(command, sizeof(command), "file \"%s\"", pathname);

  fp = popen(command, "r");
  if (!fp) {
    perror("popen");
    if (result_size > 0)
      result[0] = '\0';
    return;
  }

  if (result_size > 0)
    result[0] = '\0';

  while (fgets(buffer, sizeof(buffer), fp)) {
    strncat(result, buffer, result_size - strlen(result) - 1);
  }

  if (pclose(fp) == -1) {
    perror("pclose");
    result[0] = '\0';
  }
}

int digits_only(const char *s) {
  while (*s)
    if (isdigit(*s++) == 0)
      return 0;
  return 1;
}

void free_cbuf() {
  for (int i = 0; i < n_choices; i++) {
    free(choices[i]);
  }
  free(choices);
  choices = NULL;
  n_choices = 0;
}

int n_digits(int n);

void print_logo(WINDOW *menu_win) {
  const char *logo[] = {"      :::::::::: :::::::::: :::    :::",
                        "     :+:        :+:        :+:    :+: ",
                        "    +:+        +:+         +:+  +:+   ",
                        "   :#::+::#   +#++:++#     +#++:+     ",
                        "  +#+        +#+         +#+  +#+     ",
                        " #+#        #+#        #+#    #+#     ",
                        "###        ########## ###    ###      "};

  int n_lines = sizeof(logo) / sizeof(logo[0]);
  int start_y = LINES / 4;

  for (int i = 0; i < n_lines; i++) {
    int line_len = strlen(logo[i]);
    int start_x = (COLS - line_len) / 2;
    mvwprintw(menu_win, start_y + i, start_x, "%s", logo[i]);
  }
}

void handle_search(WINDOW *menu_win, int n_choices, int *highlight) {
  mvprintw(LINES - 1, 0, ": ");
  int c = 0;
  int count = 0;
  int max_digits = n_digits(n_choices);
  char input_buffer[32];
  int i = 0;
  input_buffer[0] = '\0';

  while (count < n_choices && c != 27 && c != 10 && c != 261 && c != 108 &&
         n_digits(count) <= n_digits(n_choices)) {
    clrtoeol();
    refresh();
    c = wgetch(menu_win);
    if ((c == KEY_BACKSPACE || c == 127) && i > 0) {
      i--;
      input_buffer[i] = '\0';
    } else if (c != 27 && c != 10 && c != 261 && c != 108) {
      if (i < 32) {
        input_buffer[i++] = (char)c;
        input_buffer[i] = '\0';
      }
    }
    mvprintw(LINES - 1, 0, ":%s", input_buffer);
    refresh();
    if (strncmp(input_buffer, "G", 1) == 0) {
      *highlight = n_choices + 1;
    } else if (strncmp(input_buffer, "gg", 2) == 0) {
      *highlight = 1;
    }else if (strncmp(input_buffer, "vim", 3) == 0) {
      char vim_cmd[6];
      endwin();
      system("vim .");
      initscr();
			break;
    }

    else if (strncmp(input_buffer, "w", 1) == 0) {
      clear();
      int _s = -6;
      wattron(menu_win, A_REVERSE);
      mvwprintw(
          menu_win, LINES + _s--, 0, "%s",
          "This is free software, and you are welcome to redistribute it under "
          "certain conditions.\nThis program is distributed in the hope that "
          "it will be useful, but WITHOUT ANY WARRANTY;\nwithout even the "
          "implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR "
          "PURPOSE.\nSee the GNU General Public License for more details: "
          "<http://www.gnu.org/licenses/>");
      mvwprintw(menu_win, LINES + _s--, 0, "%s",
                "This program comes with ABSOLUTELY NO WARRANTY;");
      mvwprintw(menu_win, LINES + _s--, 0, "%s",
                "fex 1.0 Copyright (C) 2025 Eduardo Meli");
      wattroff(menu_win, A_REVERSE);
      print_logo(menu_win);
    } else if (digits_only(input_buffer)) {
      count = atoi(input_buffer);

      if (count < n_choices) {
        *highlight = count + 1;
        if (i == max_digits - 1)
          break;
      } else {
        *highlight = n_choices + 1;
      }
    }
  }
  clrtoeol();
  refresh();
}

void load_directory(const char *dirpath) {
  DIR *dir;
  struct dirent *entry;

  if (chdir(dirpath) != 0) {
    perror("chdir");
    return;
  }

  free_cbuf();

  dir = opendir(".");
  if (!dir) {
    perror("opendir");
    exit(EXIT_FAILURE);
  }

  while ((entry = readdir(dir)) != NULL) {
    if (hide_hidden_files && entry->d_name[0] == '.' &&
        strncmp(entry->d_name, "..", 2) != 0) {
      continue;
    }
    char **temp = realloc(choices, (n_choices + 1) * sizeof(*choices));
    if (!temp) {
      perror("realloc");
      free_cbuf();
      closedir(dir);
      exit(EXIT_FAILURE);
    }
    choices = temp;
    choices[n_choices] = strdup(entry->d_name);
    if (!choices[n_choices]) {
      perror("strdup");
      free_cbuf();
      closedir(dir);
      exit(EXIT_FAILURE);
    }
    n_choices++;
  }
  closedir(dir);
}
void print_menu(WINDOW *menu_win, int highlight) {
  int x = 2, y = 2;
  int maxy, maxx;
  getmaxyx(menu_win, maxy, maxx);
  int visible_count = maxy - 2;
  int first;

  if (n_choices <= visible_count) {
    first = 0;
  } else {
    first = highlight - 1;
    if (first > (n_choices)-visible_count)
      first = (n_choices)-visible_count + 1;
  }

  werase(menu_win);

  for (int i = first; i < first + visible_count && i < n_choices; ++i) {
    struct stat st;
    lstat(choices[i], &st);
    const char *fmt;
    switch (st.st_mode & S_IFMT) {
    case S_IFLNK:
      fmt = "%d\t| {%s}";
      break;
    case S_IFDIR:
      fmt = "%d\t| [%s]";
      break;
    case S_IFCHR:
      fmt = "%d\t| ..%s..";
      break;
    case S_IFBLK:
      fmt = "%d\t| _%s_";
      break;
    default:
      fmt = "%d\t| %s";
      break;
    }
    int is_highlighted = ((highlight - 1) == i);
    if (is_highlighted)
      wattron(menu_win, A_REVERSE);
    mvwprintw(menu_win, y, x, fmt, i, choices[i]);
    if (is_highlighted)
      wattroff(menu_win, A_REVERSE);
    ++y;
  }
  wrefresh(menu_win);
}

void error(char *what) {
  fprintf(stderr, "%s\n", what);
  exit(EXIT_FAILURE);
}

void print_licensing(WINDOW *menu_win) {
  const char *t0 = "fex 1.0 Copyright (C) 2025 Eduardo Meli";
  const char *t1 = "for copyright details type `:w`.";
  mvprintw(LINES - 3, COLS - strlen(t0), "%s", t0);
  mvprintw(LINES - 4, COLS - strlen(t1), "%s", t1);
  wrefresh(menu_win);
}

int main(int argc, char **argv) {
  WINDOW *menu_win;
  int highlight = 1;
  int choice = 0;
  int c;
  char info[1024];
  if (argc < 2) {
    load_directory(".");
  } else {
    struct stat st;
    if (stat(argv[1], &st) == 0 && S_ISDIR(st.st_mode)) {
      load_directory(argv[1]);
    } else {
      error("Cannot find selected directory");
    }
  }
  initscr();
  clear();
  noecho();
  cbreak();
  startx = 0;
  starty = 1;

  menu_win = newwin(LINES, COLS, starty, startx);
  keypad(menu_win, TRUE);

  refresh();
  print_menu(menu_win, highlight);
  print_licensing(menu_win);

  refresh();
  while (1) { // main loop
    get_file_info(choices[highlight - 1], info, sizeof(info));
    mvprintw(0, 0, "%s", info);
    clrtoeol();
    refresh();

    c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
    case 'k': // 'k'
      if (highlight == 1)
        highlight = n_choices;
      else
        --highlight;
      break;
    case KEY_DOWN:
    case 'j': // 'j'
      if (highlight == n_choices)
        highlight = 1;
      else
        ++highlight;
      break;
    case 260: // arrow left
    case 'h':
      load_directory("..");
      highlight = 1;
      memset(info, 0, sizeof(info));
      break;
    case 261:  // arrow right
    case 'l':  // l
    case 10: { // enter
      struct stat st;
      if (stat(choices[highlight - 1], &st) == 0 && S_ISDIR(st.st_mode)) {
        load_directory(choices[highlight - 1]);
        highlight = 1;
        memset(info, 0, sizeof(info));
      } else {
        if (is_text_file(choices[highlight - 1])) {
          char vim_cmd[512];
          endwin();
          snprintf(vim_cmd, sizeof(vim_cmd), "vim \"%s\"",
                   choices[highlight - 1]);
          system(vim_cmd);
          initscr();
          clear();
          noecho();
          cbreak();
          menu_win = newwin(LINES, COLS, starty, startx);
          keypad(menu_win, TRUE);
          print_menu(menu_win, highlight);
          refresh();
          break;
        }
        openFile(choices[highlight - 1]);
      }
      break;
    }
    case 'q':
      choice = -1;
      break;
    case 'a':
      hide_hidden_files = !hide_hidden_files;
      load_directory(".");
      refresh();
      break;
    case ':':
      handle_search(menu_win, n_choices - 1, &highlight);
    }
    print_menu(menu_win, highlight);
    if (choice != 0)
      break;
  }

  refresh();
  endwin();

  free_cbuf();
  return 0;
}
