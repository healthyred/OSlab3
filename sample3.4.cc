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
  vm_syslog(p, 10);
}
