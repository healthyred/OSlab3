#include <iostream>
#include "vm_app.h"

using namespace std;

int main()
{
  char *p[20];
  for (int i = 0; i < 20;i++){
    p[i] = (char *) vm_extend();
    for (int j = 0; j < 10; j++){
      p[i][j] = 'o';
    }
  }

  for (int i = 0; i < 20;i++){
    vm_syslog(p[i], 2000);
  }
}
