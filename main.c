#include "xdg.h"
#include <ctype.h>
#include <dirent.h>
#include <ncurses.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#define FEX_VERSION "1.1"

void open_file(const char *filename);

int startx = 0;
int starty = 0;
char **choices = NULL;
int n_choices = 0;
bool show_hidden_files = false;

int max_val(int a, int b) { return a > b ? a : b; }

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

#define ALPHABET_SIZE 128

typedef struct trie_node {
  struct trie_node *children[ALPHABET_SIZE];
  bool is_end_of_word;
  int index;
} trie_node;

trie_node *create_trie_node() {
  trie_node *node = malloc(sizeof(trie_node));
  if (!node)
    return NULL;
  node->is_end_of_word = false;
  node->index = -1;
  for (int i = 0; i < ALPHABET_SIZE; i++)
    node->children[i] = NULL;
  return node;
}

void insert_trie(trie_node *root, const char *word, int index) {
  trie_node *current = root;
  for (int i = 0; word[i]; i++) {
    int idx = (int)word[i];
    if (idx < 0 || idx >= ALPHABET_SIZE)
      continue;
    if (current->children[idx] == NULL) {
      current->children[idx] = create_trie_node();
    }
    current = current->children[idx];
  }
  current->is_end_of_word = true;
  current->index = index;
}

trie_node *search_trie_prefix(trie_node *root, const char *prefix) {
  trie_node *current = root;
  for (int i = 0; prefix[i]; i++) {
    int idx = (int)prefix[i];
    if (idx < 0 || idx >= ALPHABET_SIZE)
      return NULL;
    if (current->children[idx] == NULL)
      return NULL;
    current = current->children[idx];
  }
  return current;
}

void collect_trie_indices(trie_node *node, int *indices, int *count,
                          int max_count) {
  if (!node || *count >= max_count)
    return;
  if (node->is_end_of_word) {
    indices[*count] = node->index;
    (*count)++;
  }
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (node->children[i])
      collect_trie_indices(node->children[i], indices, count, max_count);
  }
}

void free_trie(trie_node *node) {
  if (!node)
    return;
  for (int i = 0; i < ALPHABET_SIZE; i++) {
    if (node->children[i])
      free_trie(node->children[i]);
  }
  free(node);
}

void handle_search(WINDOW *menu_win, int *highlight) {
  trie_node *root = create_trie_node();
  for (int i = 0; i < n_choices; i++) {
    insert_trie(root, choices[i], i);
  }
  char query[256] = {0};
  int pos = 0;
  int c;
  int selected_match = 0;
  int match_count = 0;
  int indices[256] = {0};
  const int visible_count = 5;
  mvprintw(LINES - 1, 0, "/");
  refresh();
  while ((c = wgetch(menu_win)) != '\n' && c != 27) {
    if ((c == KEY_BACKSPACE || c == 127) && pos > 0) {
      pos--;
      query[pos] = '\0';
    } else if (c != '\n' && pos < (int)sizeof(query) - 1 && c != KEY_UP &&
               c != KEY_DOWN) {
      query[pos++] = (char)c;
      query[pos] = '\0';
    }
    if (c == KEY_UP || c == KEY_DOWN) {
      if (match_count > 0) {
        if (c == KEY_UP) {
          selected_match = (selected_match - 1 + match_count) % match_count;
        } else if (c == KEY_DOWN) {
          selected_match = (selected_match + 1) % match_count;
        }
      }
    }
    mvprintw(LINES - 1, 0, "/%s", query);
    clrtoeol();
    refresh();
    for (int i = LINES - (visible_count + 2); i < LINES - 1; i++) {
      move(i, 0);
      clrtoeol();
    }
    trie_node *node = search_trie_prefix(root, query);
    match_count = 0;
    if (node) {
      collect_trie_indices(node, indices, &match_count, 256);
    }
    if (match_count > 0) {
      if (selected_match >= match_count)
        selected_match = 0;
      int first_index;
      if (match_count <= visible_count) {
        first_index = 0;
      } else {
        first_index = selected_match - visible_count / 2;
        if (first_index < 0)
          first_index = 0;
        else if (first_index > match_count - visible_count)
          first_index = match_count - visible_count;
      }
      for (int i = first_index;
           i < first_index + visible_count && i < match_count; i++) {
        if (i == selected_match)
          attron(A_REVERSE);
        mvprintw(LINES - (visible_count + 1) + (i - first_index), 0, "%s",
                 choices[indices[i]]);
        if (i == selected_match)
          attroff(A_REVERSE);
      }
      *highlight = indices[selected_match] + 1;
    } else {
      mvprintw(LINES - (visible_count + 1), 0, "No matches");
    }
    refresh();
  }
  free_trie(root);
  move(LINES - 1, 0);
  clrtoeol();
  for (int i = LINES - (visible_count + 1); i < LINES - 1; i++) {
    move(i, 0);
    clrtoeol();
  }
  refresh();
}

void print_menu(WINDOW *menu_win, int highlight) {
  int x = 2, y = 2;
  int maxy = getmaxy(menu_win);
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

void print_logo(WINDOW *menu_win) {
  const char *logo[] = {"      :::::::::: :::::::::: :::    :::",
                        "     :+:        :+:        :+:    :+: ",
                        "    +:+        +:+         +:+  +:+   ",
                        "   :#::+::#   +#++:++#     +#++:+     ",
                        "  +#+        +#+         +#+  +#+     ",
                        " #+#        #+#        #+#    #+#     ",
                        "###        ########## ###    ###      "};
  clear();
  int n_lines = sizeof(logo) / sizeof(logo[0]);
  int start_y = LINES / 4;
  for (int i = 0; i < n_lines; i++) {
    int line_len = strlen(logo[i]);
    int start_x = (COLS - line_len) / 2;
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
            "fex" FEX_VERSION " Copyright (C) 2025 Eduardo Meli");
  wattroff(menu_win, A_REVERSE);
  wgetch(menu_win);
}

void handle_keyw(WINDOW *menu_win, int n_choices, int *highlight) {
  mvprintw(LINES - 1, 0, ": ");
  int c = 0;
  int count = 0;
  int max_digits_val = n_digits(n_choices);
  char input_buffer[32];
  int i = 0;
  input_buffer[0] = '\0';
  while (count < n_choices && c != 27 && c != 261 && c != 108 &&
         n_digits(count) <= n_digits(n_choices)) {
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
      if (count < n_choices) {
        *highlight = count + 1;
        if (i == max_digits_val)
          break;
      } else {
        *highlight = n_choices + 1;
      }
    }
    if (c == 10) {
      if (strncmp(input_buffer, "G", 2) == 0) {
        *highlight = n_choices + 1;
      } else if (strncmp(input_buffer, "gg", 3) == 0) {
        *highlight = 1;
      } else if (strncmp(input_buffer, "vim", 4) == 0) {
        endwin();
        pid_t pid = fork();
        if (pid == 0) {
          execlp("vim", "vim", choices[*highlight - 1], (char *)NULL);
          perror("execlp");
          exit(EXIT_FAILURE);
        } else if (pid > 0) {
          int status;
          waitpid(pid, &status, 0);
          initscr();
          clear();
          noecho();
          cbreak();
          menu_win = newwin(LINES, COLS, starty, startx);
          keypad(menu_win, TRUE);
          print_menu(menu_win, *highlight);
          refresh();
          break;
        } else {
          perror("fork");
          exit(EXIT_FAILURE);
        }
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
    if (!show_hidden_files && entry->d_name[0] == '.' &&
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

void error(char *what) {
  fprintf(stderr, "%s\n", what);
  exit(EXIT_FAILURE);
}

void print_licensing(WINDOW *menu_win) {
  const char *t0 = "fex " FEX_VERSION " Copyright (C) 2025 Eduardo Meli";
  const char *t1 = "for copyright details type `:w`.";
  mvprintw(LINES - 4, COLS - strlen(t0), "%s", t0);
  mvprintw(LINES - 3, COLS - strlen(t1), "%s", t1);
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
  while (1) {
    get_file_info(choices[highlight - 1], info, sizeof(info));
    mvprintw(0, 0, "%s", info);
    clrtoeol();
    refresh();
    c = wgetch(menu_win);
    switch (c) {
    case KEY_UP:
    case 'k':
      if (highlight == 1)
        highlight = n_choices;
      else
        --highlight;
      break;
    case KEY_DOWN:
    case 'j':
      if (highlight == n_choices)
        highlight = 1;
      else
        ++highlight;
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
      struct stat st;
      if (stat(choices[highlight - 1], &st) == 0 && S_ISDIR(st.st_mode)) {
        load_directory(choices[highlight - 1]);
        highlight = 1;
        memset(info, 0, sizeof(info));
      } else {
        if (is_text_file(choices[highlight - 1])) {
          pid_t pid = fork();
          if (pid == 0) {
            endwin();
            execlp("vim", "vim", choices[highlight - 1], (char *)NULL);
            perror("execlp");
            exit(EXIT_FAILURE);
          } else if (pid > 0) {
            int status;
            waitpid(pid, &status, 0);
            initscr();
            clear();
            noecho();
            cbreak();
            menu_win = newwin(LINES, COLS, starty, startx);
            keypad(menu_win, TRUE);
            print_menu(menu_win, highlight);
            refresh();
            break;
          } else {
            perror("fork");
            exit(EXIT_FAILURE);
          }
        }
        openFile(choices[highlight - 1]);
      }
      break;
    }
    case 'q':
      choice = -1;
      break;
    case 'a':
      show_hidden_files = !show_hidden_files;
      {
        load_directory(".");
        if (highlight > n_choices)
          highlight = n_choices;
        refresh();
      }
      break;
    case ':':
      handle_keyw(menu_win, n_choices - 1, &highlight);
      break;
    case '/':
      handle_search(menu_win, &highlight);
      break;
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
