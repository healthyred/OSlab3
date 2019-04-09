/*External Pager*/
#include "vm_pager.h"
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
using namespace std;

typedef vector<page_table_t> tables; //Vector of page tables

unsigned int memory_pages;
unsigned int disk_blocks;


void vm_init(unsigned int memory_pages, unsigned int disk_blocks){
/*Called when the pager starts, it is the number of pages provided in physical memory and the number of disk blocks avaliable on disk*/

  memory_pages = memory_pages;
  disk_blocks = disk_blocks;

}

void vm_create(pid_t pid){
/*Called when a new application starts, and the data structures it needs to handle the process, and its subsequent calls to the library.*/
};




void vm_switch(pid_t pid){};

int vm_fault(void *addr, bool write_flag){
/*called when you try to read something that is read-protect, same for write*/

}


void vm_destory(){};

void* vm_extend(){};
/*This adds pages*/


int vm_syslog(void *message, unsigned int len){};
