
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include "contiguous.h"

struct contiguous {
  struct cnode *first;
  void *upper_limit;
};

struct cnode {
  size_t nsize;
  struct cnode *prev;
  struct cnode *next;
  struct contiguous *block;
};

const int SIZEOF_CONTIGUOUS = sizeof(struct contiguous);
const int SIZEOF_CNODE = sizeof(struct cnode);



static const char STAR_STR[] = "*";
static const char NULL_STR[] = "NULL";

// maybe_null(void *p) return a pointer to "NULL" or "*",
//   indicating if p is NULL or not.
static const char *maybe_null(void *p) {
  return p ? STAR_STR : NULL_STR;
}

// gapsize(n0, n1) determine the size (in bytes) of the gap between n0 and n1.
static size_t gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  void *v0 = n0;
  void *v1 = n1;
  return (v1 - v0) - n0->nsize - sizeof(struct cnode);
}

// print_gapsize(n0, n1) print the size of the gap between n0 and n1,
//     if it's non-zero.
static void print_gapsize(struct cnode *n0, struct cnode *n1) {
  assert(n0);
  assert(n1);
  size_t gap = gapsize(n0, n1);
  
  if (gap != 0) { 
    printf("%zd byte gap\n", gap);
  }
}


// pretty_print_block(chs, size) Print size bytes, starting at chs,
//    in a human-readable format: printable characters other than backslash
//    are printed directly; other characters are escaped as \xXX
static void pretty_print_block(unsigned char *chs, int size) {
  assert(chs);
  for (int i = 0; i < size; i++) {
    printf(0x20 <= chs[i] && chs[i] < 0x80 && chs[i] != '\\'
           ? "%c" : "\\x%02X", chs[i]);
  }
  printf("\n");
}

// print_node(node) Print the contents of node and all nodes that
//    follow it.  Return a pointer to the last node.
static struct cnode *print_node(struct cnode *node) {
  while (node != NULL) {
    void *raw = node + 1;     // point at raw data that follows.
    printf("struct cnode\n");
    printf("    nsize: %ld\n", node->nsize);
    printf("    prev: %s\n", maybe_null(node->prev));
    printf("    next: %s\n",  maybe_null(node->next));

    printf("%zd byte chunk: ", node->nsize);
    
    pretty_print_block(raw, node->nsize);
    
    if (node->next == NULL) {
      return node;
    } else {
      print_gapsize(node, node->next);
      node = node->next;
    }
  }
  return NULL;
}



static void print_hr(void) {
    printf("----------------------------------------------------------------\n");
}

// print_debug(block) print a long message showing the content of block.
void print_debug(struct contiguous *block) {
  assert(block);
  void *raw = block;

  print_hr();
  printf("struct contiguous\n");
  printf("    first: %s\n", maybe_null(block->first));

  if (block->first == NULL) {
    size_t gap = block->upper_limit - raw - sizeof(struct contiguous);
    printf("%zd byte gap\n", gap);           
  } else {
    void *block_first = block->first;
    size_t gap = block_first - raw - sizeof(struct contiguous);
    if (gap) {
      printf("%zd byte gap\n", gap);
    }
  }
 
  struct cnode *lastnode = print_node(block->first);
  
  if (lastnode != NULL) {
    print_gapsize(lastnode, block->upper_limit);
  }

  print_hr();
}


// See contiguous.h for documentation.
struct contiguous *make_contiguous(size_t size) {
  assert(size > 0);
  void *my_heap = malloc(size);
  struct contiguous *start = my_heap;
  start->first = NULL;
  start->upper_limit = my_heap + size;
  char *end = my_heap + size;
  char *remain = my_heap + sizeof(struct contiguous);

  while (remain != end) {
    *remain= '$';
    ++remain;
  }
  return start;
}


// See contiguous.h for documentation.
void destroy_contiguous(struct contiguous *block) {
  assert(block);
  if (block->first != NULL) printf("Destroying non-empty block!\n");
    free(block);
  }


// See contiguous.h for documentation.
void cfree(void *p) {
  if (p == NULL) {
    return;
  }

  struct cnode *cnode = p - sizeof(struct cnode);

  if (cnode->next == NULL  && cnode->prev == NULL) {
    struct contiguous *block = cnode->block;
    block->first = NULL;
    return;
  }

  if (cnode->next == NULL) {
    cnode->prev->next = NULL;
    cnode->prev = NULL;
    return;
  }

  if (cnode->prev == NULL && cnode->next != NULL) {
    struct contiguous *block = cnode->block;
    block->first = cnode->next;
    cnode->next->prev = NULL;
    cnode->next = NULL;
    return;
  }

  cnode->prev->next = cnode->next;
  cnode->next->prev = cnode->prev;
  return;
}

// add_node(node, size, block) is a helper function to add a node
//   to the block. It will check if there is enough space to add a node.
// Requires: block is not NULL.
//         : size is non-negative integer.
// Effects: may modify block.
//        : may modify node.
// Time: O(n) where n is the number of cnodes.
static void *add_node(struct cnode *node, int size, struct contiguous *block) {
  assert(block);
  void *bottom_addr = block->upper_limit;
  void *block_addr = block;
  void *below_contiguous = block_addr + sizeof(struct contiguous);
  void *first_node = node;
  void *first_chunk = first_node + sizeof(struct cnode);
  void *second_node = node->next;
  int byte_gap_2 = second_node - (first_chunk + node->nsize);
  int byte_gap = bottom_addr - (first_chunk + node->nsize);
  int gap_below_block = first_node - below_contiguous;

  if (node->prev == NULL) {
    if (gap_below_block > (size + sizeof(struct cnode))) {
      struct cnode *new_node = below_contiguous;
      block->first = new_node;
      new_node->next = node;
      new_node->nsize = size;
      new_node->prev = NULL;
      new_node->block = block;
      node->prev = new_node;
      void *return_node = new_node;
      void *return_it = return_node + sizeof(struct cnode);
      return return_it;
    } 
  }


  if (node->next == NULL) {
    if (byte_gap > (size + sizeof(struct cnode))) {
      void *update_node = first_chunk + node->nsize;
      struct cnode *new_node = update_node;
      new_node->next = NULL;
      new_node->nsize = size;
      new_node->prev = node;
      new_node->block = block;
      node->next = new_node;
      void *return_node = new_node;
      void *return_it = return_node + sizeof(struct cnode);
      return return_it;
    } else return NULL;
  }


  if (node->next != NULL) {
    if (byte_gap_2 > (size + sizeof(struct cnode))) {
    void *add = first_chunk + node->nsize;
    struct cnode *new_node = add;
    new_node->next = node->next;
    node->next->prev = new_node;
    node->next = new_node;
    new_node->prev = node;
    new_node->nsize = size;
    new_node->block = block;
    void *return_node = new_node;
    void *return_it = return_node + sizeof(struct cnode);
    return return_it;
  } else return add_node(node->next, size, block);
}

return NULL;
}

// See contiguous.h for documentation.
void *cmalloc(struct contiguous *block, int size) {
  assert(block);
  void *bottom = block->upper_limit;
  void *block_cont = block;

  if (block->first == NULL) {
    if ((bottom - (block_cont + sizeof(struct contiguous))) > (size + sizeof(struct cnode))) {
      struct cnode *node = block_cont + sizeof(struct contiguous);
      node->block = block;
      node->next = NULL;
      node->nsize = size;
      node->prev = NULL;
      block->first = node;
      void *c_node = node;
      void *c_node_it = c_node + sizeof(struct cnode);
      return c_node_it;
    } else return NULL;
  }
  return add_node(block->first, size, block);
}


