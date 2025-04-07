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
#include "trie.h"
#include <ncurses.h>
#include <stdbool.h>
#include <stdlib.h>

trie_node *create_trie_node() {
  trie_node *node = malloc(sizeof(trie_node));
  if (node) {
    node->is_end_of_word = false;
    node->index = -1;
    for (int i = 0; i < ALPHABET_SIZE; i++)
      node->children[i] = NULL;
  }
  return node;
}

void insert_trie(trie_node *root, const char *word, int index) {
  trie_node *cur = root;
  for (int i = 0; word[i]; i++) {
    int idx = (int)word[i];
    if (idx < 0 || idx >= ALPHABET_SIZE)
      continue;
    if (!cur->children[idx])
      cur->children[idx] = create_trie_node();
    cur = cur->children[idx];
  }
  cur->is_end_of_word = true;
  cur->index = index;
}

trie_node *search_trie_prefix(trie_node *root, const char *prefix) {
  trie_node *cur = root;
  for (int i = 0; prefix[i]; i++) {
    int idx = (int)prefix[i];
    if (idx < 0 || idx >= ALPHABET_SIZE)
      return NULL;
    if (!cur->children[idx])
      return NULL;
    cur = cur->children[idx];
  }
  return cur;
}

void collect_trie_indices(trie_node *node, int *indices, int *count,
                          int max_count) {
  if (!node || *count >= max_count)
    return;
  if (node->is_end_of_word) {
    indices[*count] = node->index;
    (*count)++;
  }
  for (int i = 0; i < ALPHABET_SIZE; i++)
    if (node->children[i])
      collect_trie_indices(node->children[i], indices, count, max_count);
}

void free_trie(trie_node *node) {
  if (!node)
    return;
  for (int i = 0; i < ALPHABET_SIZE; i++)
    if (node->children[i])
      free_trie(node->children[i]);
  free(node);
}

void handle_search(WINDOW *menu_win, int *highlight, int n_choices,
                   char *choices[]) {
  trie_node *root = create_trie_node();
  for (int i = 0; i < n_choices; i++)
    insert_trie(root, choices[i], i);
  char query[BUFSIZE] = {0};
  int pos = 0, c, selected_match = 0, match_count = 0, indices[BUFSIZE] = {0};
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
        if (c == KEY_UP)
          selected_match = (selected_match - 1 + match_count) % match_count;
        else if (c == KEY_DOWN)
          selected_match = (selected_match + 1) % match_count;
      }
    }
    mvprintw(LINES - 1, 0, "/%s", query);
    clrtoeol();
    refresh();

    for (int i = LINES - (visible_count + 2); i < LINES - 1; i++) {
      move(i, 0);
      clrtoeol();
    }

    if (query[0] == '\0') {
      refresh();
      continue;
    }
    trie_node *node = search_trie_prefix(root, query);
    match_count = 0;
    if (node)
      collect_trie_indices(node, indices, &match_count, BUFSIZE);
    if (match_count > 0) {
      if (selected_match >= match_count)
        selected_match = 0;
      int first_index = (match_count <= visible_count)
                            ? 0
                            : (selected_match - visible_count / 2);
      if (first_index < 0)
        first_index = 0;
      else if (first_index > match_count - visible_count)
        first_index = match_count - visible_count;
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
    } else
      mvprintw(LINES - (visible_count + 1), 0, "No matches");
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