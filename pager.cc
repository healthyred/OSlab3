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
#include <cstring>
using namespace std;

//TODO: Syslog, Destory

typedef vector<page_table_t> tables; //Vector of page tables

struct Vpage{
  //TODO: set the dirty bits later
  int disk_block;
  int dirty;//use dirty bit to test write, set dirty bit when we modify page, when we swap back in set to 0, unless we modify again
  int zero;//use zero bit to read or not, we don't need to write to disk
  int resident;
  int ppage;
  int reference;
  int arenaidx;
};

struct process{
  vector<Vpage*> pageVector;//deallocate this vector
  page_table_t ptable;//deallocate this page table
};

process* current; //Then we look through map each time
stack<int> phys_mem;
stack<int> disk;
//map<int, page_table_t> diskMap;
map<pid_t, process*> processMap;
vector<Vpage*> clockQ; //A queue of the most recently accessed processes

void vm_init(unsigned int memory_pages, unsigned int disk_blocks){
/*Called when the pager starts, it is the number of pages provided in physical memory and the number of disk blocks avaliable on disk*/

  for (int i = 0; i < memory_pages; i++){
    phys_mem.push(i);
  }
  for (int j = 0; j < disk_blocks; j++){
    disk.push(j);
  }
  
  //cout << "Pager started with" + to_string( memory_pages) + "physical memory pages." << endl;
  
}

void vm_create(pid_t pid){
/*Called when a new application starts, and the data structures it needs to handle the process, and its subsequent calls to the library.*/

  process* newProcess = new process;
  page_table_t ptable;
  //set entries for the entire range
  for (int i = 0; i < (int) ((unsigned long)VM_ARENA_SIZE/ (unsigned long)VM_PAGESIZE); i++){
    ptable.ptes[i].ppage = -1;
    ptable.ptes[i].read_enable = 0;
    ptable.ptes[i].write_enable = 0;
  }
  newProcess->ptable = ptable;
  //process* toadd = &newProcess;
  processMap.insert(pair<pid_t, process* >(pid, newProcess));
  current = newProcess;
  //cout << "hello" <<endl;
};

void vm_switch(pid_t pid){
  //write error for calling this before vm_create
  //If there is a process, then we need to swap the process
  //Infrastrucure will call VMswitch
  page_table_t* temp = &(processMap[pid]->ptable);
  page_table_base_register = temp;
  current = processMap[pid];
};


int vm_fault(void *addr, bool write_flag){
  /*called when you try to read something that is read-protect, same for write*/

  //Find vpage given the address
  unsigned long address = (unsigned long) addr; //The current vpage we divide by 2000 to get to the address
  int vpageidx = (int) (address - (unsigned long)(VM_ARENA_BASEADDR))/ VM_PAGESIZE;
  
  Vpage* toUpdate = (current->pageVector.at(vpageidx));
  toUpdate-> arenaidx = vpageidx;
  int ppage_num;//current ppage we are on
  //get free ppage if its a non-resident and set ppage to this if we have a resident bit already
  
  if (toUpdate->resident == -1){
    if (phys_mem.empty()){
      //we need to evict from physical memory, by grabbing the first process map
      // and we evict that process
      Vpage* pageToDisk; //actually pop off later

      //if the reference bit is marked, push it back to the end of the clockQ
      while((clockQ.front() -> reference)){
        //Cycles through the clockQ wipping the reference bits to 0
        pageToDisk = clockQ.front();
        pageToDisk -> reference = 0;//whenever I set reference bit to 0, also set read/write to 0 for the pages in the clock algorithm
        clockQ.push_back(clockQ.front());
        clockQ.erase(clockQ.begin());
        //set the rw bits for each evicted page to false
        current->ptable.ptes[pageToDisk->arenaidx].write_enable = 0;
        current->ptable.ptes[pageToDisk->arenaidx].read_enable = 0;
      }
      //set dirty bit to 0, since we no longer need it, unless we actually change the contents
      pageToDisk ->dirty = 0;
      unsigned int free_page = pageToDisk->ppage;
      ppage_num = free_page;
      disk_write(pageToDisk->disk_block, free_page);
      pageToDisk->ppage = -1;

      toUpdate->resident = 1;
      toUpdate->ppage = free_page;
      
      clockQ.push_back(toUpdate);//add to the clockQ
    }
    else{
    //careful of buggy logic later, since we might need to pull
    ppage_num = phys_mem.top();
    phys_mem.pop();
    toUpdate->resident = 1;
    toUpdate->ppage = ppage_num;
    }
  }else{
    ppage_num = toUpdate->ppage;
  }
  toUpdate->reference = 1;
  clockQ.push_back(toUpdate);//push the most recently accessed page onto the stack
 
  //update page_table_t
  current->ptable.ptes[vpageidx].ppage = ppage_num;
  if(write_flag){
    current->ptable.ptes[vpageidx].write_enable = 1;
    current->ptable.ptes[vpageidx].read_enable = 1;
    current->ptable.ptes[vpageidx].dirty = 1;
  }else{
    current->ptable.ptes[vpageidx].read_enable = 1;
  }

  //cout << 3 << endl;
  //Zero when zeroPage is being used using memset
  if(toUpdate->zero){
    cout <<"Page Number: " <<  ppage_num << endl;
    memset((char *) ((unsigned long) pm_physmem + (ppage_num * VM_PAGESIZE)), 0 , VM_PAGESIZE);
    toUpdate->zero = 0;
  }

  return 0;
}


void vm_destroy(){
  /*Deallocates all of the memory, and memory of the current process
    (page tables, physical pages, and disk blocks), released physical pages need to go back onto the disk block.*/

  /*
  temp = current;


  for (vector<Vpage *>::iterator it = pageVector.begin())
  
  for(int i = 0; i < (current->pageVector).size(); i++){
    int putonstack = ((current->pageVector)->ppage);
    delete current->
    }*/

  
};



void* vm_extend(){

  //Check if the current has a vpage
  if(disk.empty()){
    return NULL;
  }

  //create vpage
  Vpage* x = new Vpage;

  //get disk block
  x->disk_block = disk.top();
  disk.pop();
  x->zero = 1;
  x->dirty = 0;
  x->resident = -1;
  x->ppage = -1;
  
  //store vpage in process
  current->pageVector.push_back(x);
  int idx = current->pageVector.size() - 1;
 
  //diskMap.insert(pair<int, page_table_t>(x.disk_block , current));

  void* address = (void*) (( idx * (unsigned long) VM_PAGESIZE) + (unsigned long) VM_ARENA_BASEADDR);
  
  return address; 
};

int vm_syslog(void *message, unsigned int len){};

