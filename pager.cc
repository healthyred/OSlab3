/*External Pager*/
#include "vm_pager.h"
#include <fstream>
#include <cstdlib>
#include <cstdio>
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <stack>
using namespace std;

typedef vector<page_table_t> tables; //Vector of page tables

struct Vpage{
  int disk_block;
  int dirty;
  int zero;
  int resident;
  int ppage;
};

struct process{
  vpage page;
  page_table_t ptable;
};

process current;
stack<int> phys_mem;
stack<int> disk;
map<int, page_table_t> diskMap;
map<pid_t, process> processMap;


void vm_init(unsigned int memory_pages, unsigned int disk_blocks){
/*Called when the pager starts, it is the number of pages provided in physical memory and the number of disk blocks avaliable on disk*/

  for (int i = 0; i < memory_pages; i++){
    phys_mem.push(i);
  }

  for (int j = 0; j < disk_blocks; j++){
    disk.push(j);
  }
  
  cout << "Pager started with" + to_string( memory_pages) + "physical memory pages." << endl;
  
}

void vm_create(pid_t pid){
/*Called when a new application starts, and the data structures it needs to handle the process, and its subsequent calls to the library.*/

  process newProcess;
  page_table_t ptable;
  ptable.ptes.ppage = -1;
  ptable.ptes.read_enable = 0;
  ptable.ptes.write_enable = 0;
  newProcess.ptable = ptable;
  processMap.insert(pair<pid_t, process>(pid, newProcess));

};

void vm_switch(pid_t pid){
  //write error for calling this before vm_create
  //If there is a process, then we need to swap the process
  //Infrastrucure will call VMswitch
  page_table_t* temp = &(processMap[pid].ptable);
  page_table_base_register = temp;
  current = processMap[pid];
  
};

int vm_fault(void *addr, bool write_flag){
  /*called when you try to read something that is read-protect, same for write*/

}


void vm_destory(){};

void* vm_extend(){};
  //Check if the current has a vpage

  if disk.empty(){
    return NULL;
  }

  //create vpage
  Vpage x;

  //pop off disk block 
  int diskblock = disk.pop();

  x.disk_block = diskblock;
  x.zero = 1;
  x.dirty = 0;
  x.resident = 0;
  x.ppage = -1; 
  //store vpage in process
  

int vm_syslog(void *message, unsigned int len){};
