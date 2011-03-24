#ifndef _osc_types_h_
#define _osc_types_h_

#define OSC_MAX_ARGS 4
#define OSC_MAX_HASHES 4
#define OSC_MAX_WORD_ARRAY_ARG 64 
#define OSC_MAX_BYTE_ARRAY_ARG 256
#define OSC_MAX_STRING_ARG 256

/* Keep these types as the bare minimum to get argument passing */
typedef enum osc_type {
  OSC_NULL=0,
  OSC_STRING,
  OSC_WORD,
  OSC_LONGWORD,
  OSC_WORD_ARRAY,
  OSC_BYTE_ARRAY,
} osc_type;

typedef struct osc_val {
  osc_type type;
  int len;
  union {
    int word;
    int word_array[OSC_MAX_WORD_ARRAY_ARG];
    unsigned char byte_array[OSC_MAX_BYTE_ARRAY_ARG];
    char str[OSC_MAX_STRING_ARG];
  } val;
} osc_val;


#ifndef __XC__
typedef struct osc_node {
  int id;
  char *name;  
  struct osc_node *parent;
  struct osc_node *child;
  struct osc_node *sibling;
  char type[OSC_MAX_ARGS];
  
  // For # nodes
  int lb;
  int ub;
  int val;

  // for sets during query matching
  struct osc_node *next;

} osc_node;

#endif

#define OSC_SET_RANGE(n,x,y) do {osc_node_ ## n ## _hash.lb = (x);osc_node_ ## n ## _hash.ub=(y);} while (0)

#define OSC_IS_HASH_NODE(n) (*((char *) n->name) == '#')

#endif // _osc_types_h_
