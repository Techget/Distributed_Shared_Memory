#include <stdio.h>
#include "sm.h"


void fatal (int nid, char *msg)
{
  printf ("node %d: Fatal internal error:\n%s\n", nid, msg);
  exit (1);
}

int main (int argc, char *argv[])
{
  int nodes, nid;

  if (sm_node_init (&argc, &argv, &nodes, &nid))
    	fatal (nid, "share: Cannot initialise!");

  sm_barrier ();

  sm_node_exit ();
  return 0;
}
