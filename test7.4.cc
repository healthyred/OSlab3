#include <iostream>
#include "vm_app.h"

using namespace std;

int main(){

  char *p;
  p = (char *) vm_extend();
  
  for (int i = 0; i < VM_PAGESIZE-2;i++){
    p[i] = '1';
  }
  p[VM_PAGESIZE-1] = '2';
  char *o;
  o = (char *) vm_extend();
  o[12] = '2';
  char *q;
  q = (char *) vm_extend();
  q[12] = '2';
  char *s;
  s = (char *) vm_extend();
  s[12] = '2';  
  char *t;
  t = (char *) vm_extend();
  t[12] = '2';

  vm_syslog(p, 8192);
}
