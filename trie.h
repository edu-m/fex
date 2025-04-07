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

trie_node *create_trie_node();
void insert_trie(trie_node *, const char *, int);
trie_node *search_trie_prefix(trie_node *, const char *);
void collect_trie_indices(trie_node *, int *, int *, int);
void free_trie(trie_node *);
void handle_search(WINDOW *, int *, int, char *[]);

#endif