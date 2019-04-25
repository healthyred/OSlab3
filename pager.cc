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
map<pid_t, process*>::iterator it;
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

  bool read = true;  
  
  Vpage* toUpdate = (current->pageVector.at(vpageidx));
  toUpdate-> arenaidx = vpageidx;
  int ppage_num;//current ppage we are on
  //get free ppage if its a non-resident and set ppage to this if we have a resident bit already

  if (toUpdate->zero){
    //Check if we need to read from disk or not
    read = false;
  }

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
      pageToDisk->resident = -1; //set the resident bit of the evicted page to nonresident

      toUpdate->resident = 1;
      toUpdate->ppage = free_page;
      
      if(read){
        //reading in content if we need it
        disk_read(toUpdate->disk_block, free_page);
	read = false;
      }
      //clockQ.push_back(toUpdate);//add to the clockQ
    }
    else{
    //if its not resident, allocate a ppage for it
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
    toUpdate->dirty = 1;
  }else{
    current->ptable.ptes[vpageidx].read_enable = 1;
  }

  //cout << 3 << endl;
  //Zero when zeroPage is being used using memset
  if(toUpdate->zero){
    //cout <<"Page Number: " <<  ppage_num << endl;
    memset((char *) ((unsigned long) pm_physmem + (ppage_num * VM_PAGESIZE)), 0 , VM_PAGESIZE);
    toUpdate->zero = 0;
  }

  
  if(read){
    //reading in content if we need it
    disk_read(toUpdate->disk_block, ppage_num);
  }
  
  return 0;
}


void vm_destroy(){
  /*Deallocates all of the memory, and memory of the current process
    (page tables, physical pages, and disk blocks), released physical pages need to go back onto the disk block.*/

  //Get each process in the map
  
  for (it = processMap.begin(); it != processMap.end();it++)
  {
    vector<Vpage *>::iterator it2;
    process* temp = it-> second;
    
    for (it2 = temp->pageVector.begin(); it2 != temp->pageVector.end();++it2)
    {
      Vpage* temp2 = *it2;
      if(temp2->ppage != -1){
	phys_mem.push(temp2->ppage);	
      }
      if(temp2->disk_block != -1){
	disk.push(temp2->disk_block);
      }
      delete temp2;
      
    }
    //delete &(temp->ptable);

    delete temp;
    processMap.erase(it);
  }
  
  //delete each Vpage from each pageVector of a process

  //release each of the physical pages back onto the disk block
  
  
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

int vm_syslog(void *message, unsigned int len){

/*Find the message address, and given the length of the thing
 and iterate through each of the Vpages and then grab it from physmem */

/*If len is 0, if message is outside of what is currently allocated in arena*/

  unsigned long address = (unsigned long) message; //The current vpage we divide by 2000 to get to the address
  int vpageidx = (int) (address - (unsigned long)(VM_ARENA_BASEADDR))/ VM_PAGESIZE;
  unsigned long endpage = address + len;
  int vpageidxend = (int) (endpage - (unsigned long)(VM_ARENA_BASEADDR))/ VM_PAGESIZE;

  //then while we have shifted less than the length
  Vpage* toReadstart = (current->pageVector.at(vpageidx));
  Vpage* toReadend = (current->pageVector.at(vpageidxend));
  Vpage* toPrint; 
  unsigned long startoffset = address - VM_ARENA_BASEADDR;

  String s;
  
  //first section
  int firstpage_num;
  firstpage_num = toReadstart->ppage;

  if (firstpage_num == -1){
    unsigned long faultaddress = ((unsigned long) vpageidx * (unsigned long) VM_PAGESIZE) + (unsigned long) VM_ARENA_BASEADDR;
    void* void_fault = (void *) faultaddress; 
    vm_fault(void_fault, false);
    firstpage_num = toReadstart->ppage;
  }

  unsigned long start = (unsigned long) pm_physmem + (firstpage_num * (unsigned long) VM_PAGESIZE) + offset;
  unsigned long end = (unsigned long) pm_physmem + ((firstpage_num + 1) * (unsigned long) VM_PAGESIZE);

  for (int idx = start; idx <= end; idx++){
    s.append(string(1, ((char *)pm_physmem)[idx]))
  } 


  //middle section, we iterate up to the end page
  for (int running = vpageidx; vpageidx < vpageidxend; running++){
    //iterate through VM and pull everything from the ppage, if its the first page, consider the offset
    //if its the last page also only pull the offset
    toPrint = current->pageVector.at(running);
    int ppage_num = toPrint->ppage;

    if (ppage_num == -1){
      unsigned long faultaddress = ((unsigned long) running * (unsigned long) VM_PAGESIZE) + (unsigned long) VM_ARENA_BASEADDR;
      void* void_fault = (void *) faultaddress; 
      vm_fault(faultaddress, false);
      ppage_num = toPrint->ppage;
    }

    unsigned long start_middle = (unsigned long) pm_physmem + (ppage_num * (unsigned long) VM_PAGESIZE);
    unsigned long end_middle = start_middle + (unsigned long) VM_PAGESIZE;
    for (int idx = start_middle; idx <= end_middle; idx++){
      s.append(string(1, ((char *)pm_physmem)[idx]))
    } 
  }
  
  //last section, only go through if its different from the first page
  if(vpageidx != vpageidxend){
      int endpage_num;
      endpage_num = toReadstart->ppage;

      if (endpage_num == -1){
        unsigned long faultaddress = ((unsigned long) vpageidxend * (unsigned long) VM_PAGESIZE) + (unsigned long) VM_ARENA_BASEADDR;
        void* void_fault = (void *) faultaddress; 
        vm_fault(void_fault, false);
        endpage_num = toReadstart->ppage;
      }

      unsigned long start = (unsigned long) pm_physmem + (endpage_num * (unsigned long) VM_PAGESIZE);
      unsigned long end = (unsigned long) pm_physmem + ((endpage_num + 1) * (unsigned long) VM_PAGESIZE) + offset;

      for (int idx = start; idx <= end; idx++){
        s.append(string(1, ((char *)pm_physmem)[idx]))
      } 
  }


  cout << "syslog \t\t\t" << s << endl;

  return 0;
};