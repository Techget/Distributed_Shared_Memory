

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
  char *sharedChar;

  if (sm_node_init (&argc, &argv, &nodes, &nid))
    fatal (nid, "share: Cannot initialise!");

  /* first, node #0 allocates a shared variable and uses it to communicate
   * the letter `A' to node #1
   */
  if (0 == nid) {
    sharedChar = (char *) sm_malloc (sizeof (char));
    *sharedChar = 'A';
  }
  sm_bcast ((void **) &sharedChar, 0);
  /* Checkpoint A */
  printf ("node %d: 1st shared variable is at %p.\n", nid, sharedChar);
  if (0 != nid)
    printf ("node %d: Value in 1st shared variable is %d\n", 
      nid, *sharedChar);
  /* Checkpoint B */
  sm_barrier ();


  sm_node_exit ();
  return 0;
}