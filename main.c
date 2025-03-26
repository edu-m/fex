#include "xdg.h"
#include <dirent.h>
#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

int startx = 0;
int starty = 0;

char **choices = NULL;
int n_choices = 0;

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

void free_cbuf() {
  for (int i = 0; i < n_choices; i++) {
    free(choices[i]);
  }
  free(choices);
  choices = NULL;
  n_choices = 0;
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
  
  if (n_choices - 1 <= visible_count) {
    first = 1;
  } else {
    first = highlight - 1;
    if (first < 1)
      first = 1;
    if (first > (n_choices - 1) - visible_count + 1)
      first = (n_choices - 1) - visible_count + 1;
  }
  
  werase(menu_win);
  
  for (int i = first; i < first + visible_count && i < n_choices; ++i) {
    if ((highlight - 1) == i) {
      wattron(menu_win, A_REVERSE);
      mvwprintw(menu_win, y, x, "%s", choices[i]);
      wattroff(menu_win, A_REVERSE);
    } else {
      mvwprintw(menu_win, y, x, "%s", choices[i]);
    }
    ++y;
  }
  wrefresh(menu_win);
}
int main(void) {
  WINDOW *menu_win;
  int highlight = 2;
  int choice = 0;
  int c;
  char info[1024];

  load_directory(".");

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

  refresh();
  while (1) { // main loop
    get_file_info(choices[highlight - 1], info, sizeof(info));
    mvprintw(0, 0, "%s", info);
    clrtoeol();
    refresh();

    c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
    case 107: // 'k'
      if (highlight == 2)
        highlight = n_choices;
      else
        --highlight;
      break;
    case KEY_DOWN:
    case 106: // 'j'
      if (highlight == n_choices)
        highlight = 2;
      else
        ++highlight;
      break;
    case 260: // arrow left
    case 104:
      load_directory("..");
      highlight = 2;
      memset(info, 0, sizeof(info));
    case 261:  // arrow right
    case 108:  // l
    case 10: { // enter
      struct stat st;
      if (stat(choices[highlight - 1], &st) == 0 && S_ISDIR(st.st_mode)) {
        load_directory(choices[highlight - 1]);
        highlight = 2;
        memset(info, 0, sizeof(info));
      } else {
        char command[256];
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
    case 113: // 'q'
      choice = -1;
      break;
    default: // just for debug!
      mvprintw(LINES-1, 0, "Character pressed is = %3d ('%c')", c, c);
      refresh();
      break;
    }
    print_menu(menu_win, highlight);
    if (choice != 0)
      break;
  }

  clrtoeol();
  refresh();
  endwin();

  free_cbuf();
  return 0;
}
