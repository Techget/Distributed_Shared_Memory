#include <stdio.h>
#include "sm.h"


void fatal (int nid, char *msg)
{
  printf ("node %d: Fatal internal error:\n%s\n", nid, msg);
  exit (1);
}

int main (int argc, char *argv[])
{
  int   nodes, nid;
  char *sharedChar, *sharedChar2;

  if (sm_node_init (&argc, &argv, &nodes, &nid))
    fatal (nid, "share: Cannot initialise!");

  /* first, node #0 allocates a shared variable and uses it to communicate
   * the letter `A' to node #1
   */
  if (0 == nid) {
    sharedChar = (char *) sm_malloc (sizeof (char));
    *sharedChar = 'A';
    printf("node %d: sharedChar is at %p. with value: %c\n", nid, sharedChar, *sharedChar);
  }
  if (1 == nid) {
    sharedChar2 = (char *) sm_malloc (sizeof (char));
    *sharedChar2 = 'B';
    printf("node %d: sharedChar2 is at %p. with value: %c\n", nid, sharedChar2, *sharedChar2);
  }


  sm_bcast((void **) &sharedChar, 0);
  sm_bcast((void **) &sharedChar2, 1);

  printf("sharedChar: %p\n", sharedChar);
  printf("sharedChar2: %p\n", sharedChar2);

  if (0 == nid) {
    printf("nid 0 try to print out *sharedChar2: %c\n", *sharedChar2);
  }
  // /* Checkpoint A */
  // printf ("node %d: 1st shared variable is at %p.\n", nid, sharedChar);
  // if (0 != nid)
  //   printf ("node %d: Value in 1st shared variable is %d\n", 
  //     nid, *sharedChar);
  sm_barrier ();


  printf("try to print out *sharedChar: %c\n", *sharedChar);

  sm_barrier ();

  sm_node_exit ();
  return 0;
}
