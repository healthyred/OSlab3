#include <iostream>
#include "vm_app.h"

using namespace std;

int main()
{
    char *p;
    p = (char *) vm_extend();
    p[200] = 'h';
    p[6000] = 'e';
    p[6001] = 'l';
    p[7000] = 'l';
    p[7643] = 'o';
    p[8000] = '!';
    vm_syslog(p, 8001);
}
