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
#ifndef TRIE_H
#define TRIE_H

#define ALPHABET_SIZE 128
#ifndef BUFSIZE
#define BUFSIZE 1024

#include <ncurses.h>
#endif

typedef struct trie_node {
  struct trie_node *children[ALPHABET_SIZE];
  bool is_end_of_word;
  int index;
} trie_node;

trie_node *create_trie_node(void);
void insert_trie(trie_node *, const char *, int);
trie_node *search_trie_prefix(trie_node *, const char *);
void collect_trie_indices(trie_node *, int *, int *, int);
void free_trie(trie_node *);
void handle_search(WINDOW *, int *, int, char *[]);

#endif
