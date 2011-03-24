
#include <stdlib.h>
#include <stdio.h>
#include "osc_types.h"

extern osc_node *osc_root;


static osc_node *init_working_set(osc_node *root)
{  
  osc_node *working_set = NULL;
  while (root != NULL) {
    root->next = working_set;
    working_set = root;      
    root = root->sibling;
  }
  return working_set;
}

static osc_node *add_children(osc_node *set, osc_node *p)
{
  p = p->child;
  while (p != NULL) {
    p->next = set;
    set = p;
    p = p->sibling;
  }
  return set;
}

static osc_node *add_to_set(osc_node *set, osc_node *p)
{
  p->next = set;
  set = p;
  return set;
}

#if 0
static void print_set(osc_node *p) 
{
  printf("{");
  while (p != NULL) {
    printf("%d (%s), ",p->id,p->name);
    p = p->next;
  }
  printf("}\n");
}
#endif

static int match(osc_node *p, char *query) {
  int match = 1;
  char *name = p->name;
  char *saved_name = NULL;
  char *saved_query = NULL;
  int finished = 0;
  int hash_matched = 0;

  while (!finished) {
    //    printf(": %s : %s :\n",query, name);
    switch (*query) 
      {
      case 0:
      case '/':
        if (*name == 0 || (OSC_IS_HASH_NODE(p) && hash_matched)) {
          finished = 1;
        }
        else if (saved_query && saved_name != name-1) {
          query = saved_query;
          saved_name++;
          name = saved_name;
        }
        else {
          finished = 1;
          match = 0;
        }
        break;      
      case '*':
        saved_query = query;
        saved_name = name;
        break;
      case '0':
      case '1':
      case '2':
      case '3':
      case '4':
      case '5':
      case '6':
      case '7':
      case '8':
      case '9':
        if (OSC_IS_HASH_NODE(p)) {
          p->val = *query-'0';          
          hash_matched = 1;
          break;
        }
      default:
        if (*query == *name || *query == '?') {
          name++;
        }
        else if (saved_query && *name != 0) {
          query = saved_query;
          saved_name++;
          name = saved_name;
        }
        else {
          finished = 1;
          match = 0;
        }
        break;
      }
    query++;
  }

  return (match);
}



osc_node *osc_match(char *query)
{
  osc_node *working_set = NULL;
  osc_node *saved = NULL;
  int finished = 0;
  char *next_query;
  osc_node *p;
  osc_node *child_set = NULL;
  osc_node *matched_set = NULL;

  working_set = init_working_set(osc_root);
  //  printf("init_set:");print_set(working_set);

  while (*query) {
    int matched;   
    child_set = NULL;
    matched_set = NULL;

    //    printf("query: %s\n", query);
    next_query=query;
    while (*next_query != 0 && *next_query != '/') 
      next_query++;   
    if (*next_query) next_query++;

    matched_set = NULL;
    child_set = NULL;
    p = working_set;
    while (p != NULL) {
      osc_node *next = p->next;

      if (match(p, query)) {
        matched_set = add_to_set(matched_set, p);
        child_set = add_children(child_set, p);
      }
      
      p = next;
    }

    //    printf("child:");print_set(child_set);
    working_set = child_set;
    //    print_set(working_set);
    query = next_query;
  }
  //  printf("matched:");print_set(matched_set);
  return matched_set;
}

