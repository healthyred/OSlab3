#include <iostream>
#include "vm_app.h"

using namespace std;

int main()
{
  char *p;
  p = (char *) vm_extend();
  p[0] = 'h';
  p = (char *) vm_extend();
  p[1] = 'e';
  p = (char *) vm_extend();
  p[2] = 'l';
  p = (char *) vm_extend();
  p[3] = 'l';
  p = (char *) vm_extend();
  p[4] = 'o';
  p[1] = 'e';
  char *q;
  q = (char *) vm_extend();
  q[0] = 'w';
  q[2] = ' ';
  q[9000] = 'c';
  char *a;

  a = (char *) vm_extend();
  char *b;
  b = (char *) vm_extend();
  char *c;
  c = (char *) vm_extend();
  
  vm_syslog(p, 10);
  vm_syslog(q,20000);
  vm_syslog(p, -1);
}
