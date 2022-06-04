#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;
struct spinlock atomic_lock;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);
/////////////////*2022-04-07*////////////
// info for Stride scheduliing
struct num_Stride{
  int CPU_share_Stride; // Sum of cpu_share of each stride_mode process
  double PASS_MLFQ;        // used cpu_time for MLFQ scheduling
  double PASS_Stride;      // used cpu_time for Stride scheduling
} num_Stride;
int max_stride_proc_pass = 0; //maximum pass in all stride processes

// fun to check what schedule mode is
enum Proc_mode
sche_mode(void){
  if((num_Stride.CPU_share_Stride == 0)||(num_Stride.PASS_MLFQ <= num_Stride.PASS_Stride))
	return MLFQ;
  else
	return Stride;
}

struct proc*
find_runnable_thread(struct proc* mother){
  enum Proc_mode schedule_mode = MLFQ;
  enum MLFQ_level lv= LOW;
  int tick = 1000;
  struct proc* p;
  struct proc* rp = mother;
  int min_PASS = 2147483645;
  // choose sched_mode 
  if(mother->CPU_share_Stride == 0 || mother->PASS_MLFQ <= mother->PASS_Stride){
    schedule_mode = MLFQ;
  }
  else{
    schedule_mode = Stride;
  }
  // find thread depends on choosen mode
  if(schedule_mode == MLFQ){    
      //find_runnable_MLFQ();
      // Find the process which is runnable by MLFQ Policy
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		  if( p->state == RUNNABLE && p->proc_mode == MLFQ && p->isthread == 1 &&p->mother == mother){
          //The scheduler chooses the next process that’s ready from the MLFQ. 
          //If any process is found in the higher priority queue, 
          //a process in the lower queue cannot be selected 
          //until the upper level queue becomes empty.
            if(p->MLFQ_lv > lv){
              rp = p;
              lv = p->MLFQ_lv;
              tick = p->tick;
            }
            else if(p->MLFQ_lv == lv){
          ///Each level adopts the Round Robin policy 
          ///with a different time quantum.
              if(lv == HIGH){
                  if((p->tick/5) >= (tick/5))
                    continue;
              }
              else if(lv == MID){
                  if((p->tick)/10 >= (tick/10))
                    continue;
              }
              else if(lv == LOW){
                  if((p->tick)/20 >= (tick/20))
                    continue;
              }
              rp = p;
              tick = p->tick;
			  
            }
          } 
          if( p->state == RUNNABLE && p == mother){
          //The scheduler chooses the next process that’s ready from the MLFQ. 
          //If any process is found in the higher priority queue, 
          //a process in the lower queue cannot be selected 
          //until the upper level queue becomes empty.
            if(p->m_MLFQ_lv > lv){
              rp = p;
              lv = p->m_MLFQ_lv;
              tick = p->m_tick;
            }
            else if(p->m_MLFQ_lv == lv){
          ///Each level adopts the Round Robin policy 
          ///with a different time quantum.
              if(lv == HIGH){
                  if((p->m_tick/5) >= (tick/5))
                    continue;
              }
              else if(lv == MID){
                  if((p->m_tick)/10 >= (tick/10))
                    continue;
              }
              else if(lv == LOW){
                  if((p->m_tick)/20 >= (tick/20))
                    continue;
              }
              rp = p;
              tick = p->m_tick;
            }
          } 


    }
  }    
  else if(schedule_mode == Stride){
      //p = find_runnable_Stride();
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state == RUNNABLE && p->proc_mode == Stride && p->isthread == 1 && p->mother == mother){
        if(p->PASS < min_PASS){
          rp = p;
          min_PASS = p-> PASS;
        }
      }
    }
  }    
  
  return rp;  
}

void
adjust_thread_state(struct proc* thread){
    struct proc *p;
    struct proc *mother =0;

    if(thread ->hasthread == 1){
	  mother = thread;
      //adjust_MLFQ_state(p);
      // Adjust MLFQ state incluing incresing tick, changing lv of MLFQ
	  thread->m_tick++;
	  thread->total_ticks_MLFQ++;
	  //Each queue has a different time allotment.
	  if(thread->m_MLFQ_lv == HIGH && thread->m_tick >= 20){
	    thread->m_MLFQ_lv = MID;
	    thread->m_tick = 0;
		// for debug
	    //cprintf("adjust-h %d\n",p->pid); 
	  }
	  else if(thread->m_MLFQ_lv == MID && thread->m_tick >= 40){
	    thread->m_MLFQ_lv = LOW;
        thread->m_tick = 0;
        // for debug
        //cprintf("adjust-m %d\n",p->pid); 
	  } 
      //init_MLFQ_state();
      // priority boosting
      if(thread->total_ticks_MLFQ >= 100){
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		  if(p->state == RUNNABLE && p->proc_mode == MLFQ && p->isthread == 1  && p->mother == mother){
			p->tick = 0;
			p->MLFQ_lv = HIGH;
		  }
          if(p->state == RUNNABLE || p == thread){
			p->m_tick = 0;             
			p->m_MLFQ_lv = HIGH;
		  }
		}
		thread->total_ticks_MLFQ = 0;
	  }


    }
    else if(thread->proc_mode == MLFQ){
	  mother = thread->mother;
      //adjust_MLFQ_state(p);
      // Adjust MLFQ state incluing incresing tick, changing lv of MLFQ
      thread->tick++;
      thread->mother->total_ticks_MLFQ++;
      //Each queue has a different time allotment.
      if(thread->MLFQ_lv == HIGH && thread->tick >= 20){ 
        thread->MLFQ_lv = MID; 
        thread->tick = 0; 
        // for debug
        //cprintf("adjust-h %d\n",p->pid); 
      }    
      else if(thread->MLFQ_lv == MID && thread->tick >= 40){ 
        thread->MLFQ_lv = LOW; 
        thread->tick = 0; 
        // for debug
        //cprintf("adjust-m %d\n",p->pid); 
      }    
      //init_MLFQ_state();
      // priority boosting
      if(thread->mother->total_ticks_MLFQ >= 100){
        for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
          if(p->state == RUNNABLE && p->proc_mode == MLFQ && p->isthread == 1 && p->mother == mother){
            p->tick = 0; 
            p->MLFQ_lv = HIGH;
          }
          if(p->state == RUNNABLE || p == thread->mother){
            p->m_tick = 0;     
            p->m_MLFQ_lv = HIGH;
		  }
        }    
        thread->mother->total_ticks_MLFQ = 0; 
      }  

	  
    }
    else if(thread->proc_mode  == Stride){
	  mother = thread -> mother;
      //adjust_Stride_state(p);
      //Adjust process pass
	  // adjust current process pass
	  thread->PASS += (10000/(thread->CPU_SHARE));
	  // adjust max_stride_proc_pass
	  if(thread->PASS > thread->mother->max_stride_thread_pass){
		thread->mother->max_stride_thread_pass = thread-> PASS;
	  }

    }

  //revaluate_PASS(schedule_mode,p);
  
  int CPU_S = mother->CPU_S;
  // if cpu share for stride is 0, and scheduling sys keep running,
  // then cpu_share_gap between  real-time and intended keep increasing.
  // to prevent it, we should init pass_MLFQ and pass_Stride
  // when cpu_share for stride is 0 
  if(CPU_S == 0){
    mother->PASS_MLFQ = 0;
    mother->PASS_Stride = 0;
    // max_stride_proc_pass should be 0 when CPU-s = 0.
    mother->max_stride_thread_pass = 0;
  }
  // revaluate cpu_time schedule_mode used
  else if(thread == mother|| thread->proc_mode == MLFQ){
    mother->PASS_MLFQ += (10000/(mother->CPU_SHARE-CPU_S));
  }
  else if(thread->proc_mode == Stride){
    mother->PASS_Stride += (10000/CPU_S);
  }


}

//Find the process which is runnable by Stride Policy
struct proc*
find_runnable_Stride(void){
  struct proc* p;
  int min_PASS = 2147483645;
  struct proc* rp;
  rp = &ptable.proc[0];
  // choose the process with the least pass
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if((p->state == RUNNABLE ) && p->proc_mode == Stride ){
	  if(p->PASS < min_PASS){
		rp = p;
		min_PASS = p-> PASS;	  
	  }
    }
  }
  return rp;
}

//fun to adjust the current process pass
void
adjust_Stride_state(struct proc* p){
  // adjust current process pass
  p->PASS += (10000/(p->CPU_SHARE));  
  // adjust max_stride_proc_pass
  if(p->PASS > max_stride_proc_pass){
	max_stride_proc_pass = p-> PASS;
  }
}

// fun to revaluate Cpu_time schedule_mode use
void
revaluate_PASS(enum Proc_mode schedule_mode,struct proc* p){
  int CPU_S = num_Stride.CPU_share_Stride;
  // if cpu share for stride is 0, and scheduling sys keep running,
  // then cpu_share_gap between  real-time and intended keep increasing.
  // to prevent it, we should init pass_MLFQ and pass_Stride
  // when cpu_share for stride is 0 
  if(CPU_S == 0){
	num_Stride.PASS_MLFQ = 0;
    num_Stride.PASS_Stride = 0;
    // max_stride_proc_pass should be 0 when CPU-s = 0.
    max_stride_proc_pass = 0;
  }
  // revaluate cpu_time schedule_mode used
  else if(schedule_mode == MLFQ){
	num_Stride.PASS_MLFQ += (10000/(100-CPU_S));
  }
  else if(schedule_mode == Stride){
    num_Stride.PASS_Stride += (10000/CPU_S);
  }
}

/////////////////*2022-04-01*////////////
struct {
  int total_ticks_MLFQ;
} num_MLFQ;

// Find the process which is runnable by MLFQ Policy
struct proc*
find_runnable_MLFQ(){
  enum MLFQ_level lv= LOW;
  int tick = 1000;
  struct proc* p;
  struct proc* rp;
  rp = ptable.proc;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	if( (p->state == RUNNABLE) && p->proc_mode == MLFQ){
	      //The scheduler chooses the next process that’s ready from the MLFQ. 
	      //If any process is found in the higher priority queue, 
	      //a process in the lower queue cannot be selected 
	      //until the upper level queue becomes empty.
          if(p->MLFQ_lv > lv){
			rp = p;
            lv = p->MLFQ_lv;
			tick = p->tick;	
          }
		  else if(p->MLFQ_lv == lv){
		  ///Each level adopts the Round Robin policy 
		  ///with a different time quantum.
			if(lv == HIGH){
				if(p->tick >= tick)
				  continue;
			}
			else if(lv == MID){
				if((p->tick)/2 >= (tick/2))
				  continue;
			}
			else if(lv == LOW){
				if((p->tick)/4 >= (tick/4))
				  continue;
			}
			rp = p;
			tick = p->tick;
		  }
	}     
  }
  //if(rp == ptable.proc && tick == 1000){
  //	panic("can't find mlfq\n");
  //}
  return rp;
}
// Adjust MLFQ state incluing incresing tick, changing lv of MLFQ
void
adjust_MLFQ_state(struct proc *p){
  p->tick++;
  num_MLFQ.total_ticks_MLFQ++;
  //Each queue has a different time allotment.
  if(p->MLFQ_lv == HIGH && p->tick >= 5){
	p->MLFQ_lv = MID;
    p->tick = 0;
	// for debug
	//cprintf("adjust-h %d\n",p->pid); 
  }
  else if(p->MLFQ_lv == MID && p->tick >= 10){
	p->MLFQ_lv = LOW;
    p->tick = 0;
	// for debug
	//cprintf("adjust-m %d\n",p->pid); 
  }
}

// priority boosting
void
init_MLFQ_state(){
  //To prevent starvation, priority boosting needs to be performed periodically
  if(num_MLFQ.total_ticks_MLFQ >= 100){
    struct proc *p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	  if(p->state == RUNNABLE){
		p->tick = 0;    
        p->MLFQ_lv = HIGH;	
	  }		
	}
    num_MLFQ.total_ticks_MLFQ = 0;

  }
}




/**/
void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
  initlock(&atomic_lock, "atomic");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  /// 2022.03.31 init MLFQ state
  p->tick = 0;
  p->MLFQ_lv = HIGH;
  /// 2022.04.07 init Stride state
  p-> proc_mode = MLFQ;
  p-> CPU_SHARE = 0;
  p-> PASS = 0;
  /// 2022.05.14 init thread state
  p-> hasthread = 0;
  p-> isthread = 0;
  p-> exiting = 0;

  p->CPU_share_Stride = 0;
  p-> PASS_MLFQ = 0;
  p-> PASS_Stride = 0;
  p-> max_stride_thread_pass = 0;
  p-> total_ticks_MLFQ = 0;
  p->CPU_S = 0;
  p->m_tick = 0;
  p->m_MLFQ_lv = HIGH;



  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];
  
  ///2022-04-01
  // initalize MLFQ state (total ticks)
  num_MLFQ.total_ticks_MLFQ = 0;
  
  ///2022-04-07
  // initalize Stride state
  num_Stride.CPU_share_Stride = 0;
  num_Stride.PASS_MLFQ = 0;
  num_Stride.PASS_Stride = 0;


  p = allocproc();
  cprintf("user\n");  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();
  // 2022-05-23
  // If this process is mother process for threads,
  // We should fork all other threads belonging to this process.

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }



  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;
  if(curproc-> hasthread == 1){
    if(fork_child(np,curproc)== -1)
	  return -1;
  }
  release(&ptable.lock);

  return pid;
}

int 
fork_child(struct proc* new, struct proc* old){
  int cnt = 0;
  int i;
  struct proc* p;
  struct proc* np;
  struct proc* curproc;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != old)
		continue;
      else
		cnt++;
  }
  if(cnt == 0)
	return 0;
  struct proc* newthreads[cnt];
  struct proc* oldthreads[cnt];
  for(p = ptable.proc,i=0; p < &ptable.proc[NPROC]; p++,i++){
      if(p->parent != old)
        continue;
      else
        p = oldthreads[i];
  }

  for(i=0;i<cnt;i++){
    if((newthreads[i] = allocproc()) == 0){
      return -1;
    }
  }
  for(i=0;i<cnt;i++){
        // Copy process state from proc.
  np = newthreads[i];
  curproc = oldthreads[i];
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  np->mother = new;

  }

  return 1;  

}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  void* dum_retval;
  int fd;
  //uint sp;
  //int arg = 0;

  if(curproc == initproc)
    panic("init exiting");
  // 2022-04-07 
  // revaluate CPU_share_Stride
  // 2022-05-22
  // if lwp call exit, all lwp belonging to it's mother should be terminated and mother also should be terminated.
  
  //1. mother process exit case
  //all lwp should call exit and mother process should call join.
  if(curproc->hasthread == 1){
	curproc->exiting = 1;
    // Scan through table looking for exited children.
	RE:
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	  if(p->parent != curproc)
		continue;
      if(p->state == ZOMBIE){
	//	cprintf("%d\n",p->pid);
        thread_join(p->pid,&dum_retval);
	//	cprintf("%d join_s\n",p->pid);
      }
	  else{
		p->killed = 1;
		wakeup1(p);
	  }
    }
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue; 
        
      else{
	//	  cprintf("fail\n");
          goto RE;
      }
    }
  }
  //2. child thread exit case
  // we should call exit in mother process.
  if(curproc->isthread == 1){
	if(curproc->parent->exiting != 1){
		acquire(&ptable.lock);
		curproc->parent->killed = 1;
		curproc->parent->exiting = 1;
		release(&ptable.lock);
	//	cprintf("%d exit_s\n",curproc->pid);
		
	}
  }
  

  if(curproc -> proc_mode == Stride)
	num_Stride.CPU_share_Stride = num_Stride.CPU_share_Stride - (curproc -> CPU_SHARE);
  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);


  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  ///2022-04-07
  enum Proc_mode schedule_mode = MLFQ;
 
  for(;;){
    // Enable interrupts on this processor.
    sti();
	
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
	
	////2022-04-07/////////////
	schedule_mode = sche_mode();
	
	if(schedule_mode == MLFQ){    
	  ////2022-04-01//////////////    
	  p = find_runnable_MLFQ();  
	  // Find the process which is runnable by MLFQ Policy
    }
	else if(schedule_mode == Stride){
	  p = find_runnable_Stride();
	} 
	else {
	  p = find_runnable_MLFQ();
	} 
    
    // If lwpgroup is choosen, 
	//find the thread which is runnable by the policy

    
    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
    //cprintf("swith to %d",p->pid);
    p->state = RUNNING;
	swtch(&(c->scheduler), p->context);	
    switchkvm();
    


	if(schedule_mode == MLFQ){
      ///2022-04-01///////////////////
      adjust_MLFQ_state(p);
      // Adjust MLFQ state incluing incresing tick, changing lv of MLFQ
      init_MLFQ_state();
      // priority boosting
	}
	else if(schedule_mode == Stride){
	  adjust_Stride_state(p);
	  //Adjust process pass
	}

	revaluate_PASS(schedule_mode,p);
    //revaluate Pass(cpu_time) schedule_mode use

    // Process is done running for now.
    // It should have changed its p->state before coming back.
    c->proc = 0;

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.

// 2022.03.31 change datatype of yield fun (void to int)
int
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;
  sched();
  release(&ptable.lock);
  return 0;
}


// 2022-04-08
// implement getlev and set_cpu_share
// getlev is a syscall that gives you the lv of the current process in MLFQ
int 
getlev(void)
{
  enum MLFQ_level lv = myproc() -> MLFQ_lv;
  if(lv == LOW){
	return 0;
  }
  else if(lv == MID){
	return 1;
  }
  else if(lv == HIGH){
	return 2;
  }
  else{
	return -1;
  }
}
// 2022-04-08
// set_cpu_share is a syscall 
// that make current process Stride schedule_mode and 
// set current process's cpu share
int
set_cpu_share(int i){
  // logic of checking wheather requested cpu_share is possible 
  // is slightly different depending on the mode of process
  if (i <= 0)
    // Error, CPU share for Stride cannnot lower than positive int
	return -1;
  
  // current proccess mode is not Stride mode
  if(myproc()->proc_mode != Stride){
	if(num_Stride.CPU_share_Stride + i > 80){
	  //Error, CPU share for Stride cannot exceed 80%.	
	  return -1;
	}
    else{
      // Set INFO about Stride scheduling for the process
	  myproc()->CPU_SHARE = i;
	  num_Stride.CPU_share_Stride =  num_Stride.CPU_share_Stride + i;
	  // changing mode with Stride
	  myproc()->proc_mode = Stride;
	  // if new stride_proc pass is 0 and if stride scheduling has continued
	  // then cpu_share_gap between  real-time and intended
	  // As long as the stride scheduling continues, cpu_share will be larger than intended
	  // to prevent that situation, we should init new proc pass to "max_stride_proc_pass" not 0
	  myproc()->PASS = max_stride_proc_pass;
	  
	  return 0;
	}
  }
  // current proccess mode is alreadly Stride mode
  else{
	if((num_Stride.CPU_share_Stride + i - (myproc()->CPU_SHARE)) > 80){
      //Error, CPU share for Stride cannot exceed 80%.
	  return -1;
	}
	else{
	  // Change INFO about Stride scheduling for the process
	  myproc()->CPU_SHARE = i;
	  num_Stride.CPU_share_Stride =  num_Stride.CPU_share_Stride + i - (myproc()->CPU_SHARE);
	  // similar reason to above part (init new proc pass to "max_stride_proc_pass")
	  myproc()->PASS = max_stride_proc_pass;
	  return 0;
	}
  }

}
//2022-05-12
//implement Basic LWP Operation - Create,Join, and Exit

//thread_create is syscall
//method to create threads within the process.
//From that point on, the execution routine assigned to each thread starts.
int
thread_create(thread_t * thread,void * (*start_routine)(void *),void *arg)
{ //cprintf("%dcreate\n",&thread);
  uint sz,sp,i,ustack[4];
  pde_t *pgdir;
  struct proc *np;
  struct proc *curproc = myproc();

  if(curproc->hasthread == 0){
	for(i = 0;i < NTHREAD ;i++){
	  curproc->sz_thread[i] = 0;
	  curproc->ex_thread[i] = 0;
    } 
    curproc->hasthread =1;
  }


  // Allocate process for thread.
  if((np= allocproc()) == 0)
    goto bad;
  // Share memoryspace(pgdir) with parent
  np->pgdir = curproc->pgdir;
  pgdir = curproc -> pgdir;
  *np->tf = *curproc->tf;
  // Allocate two pages at the next page boundary.
  // Make  the user stack.
  for(i = 0;i < NTHREAD ;i++){
	if(curproc->ex_thread[i] == 0){
		if(curproc->sz_thread[i] == 0){
		  if(i==0){
			sz = curproc-> sz;
                  }
		  else{
			sz = curproc->sz_thread[i-1];
			sz += PGSIZE;
		  } 
		  acquire(&ptable.lock);
		  if((sz = allocuvm(pgdir, sz, sz + 3*PGSIZE)) == 0)
			goto bad;
		  release(&ptable.lock);	
		  sz = sz - PGSIZE;
		  curproc->sz_thread[i] = sz;
		}
      np->sz =  curproc->sz_thread[i];
      curproc->ex_thread[i] = 1;
      np-> thread_num = i;
      break;
    }
  }
  //*np->tf = *curproc->tf;

  sp = np->sz;  
  

  // Push argument.
  ustack[3]=(uint)arg; 
  ustack[2]=0;
  ustack[1]=(uint)ustack[3];
  ustack[0]=0xffffffff;
	
  sp-=16;
  if(copyout(np->pgdir,sp,ustack,16)<0)
    goto bad;

  // insert info about it's userstackpointer and start_routine to thread
  np->tf->esp = sp;
  np->tf->eip = (uint)start_routine;
  np->tf->eax=0;

  // init thread id  
  *thread = np->pid;

  // init thread state
  np -> isthread = 1;
  np -> ret_val = 0;
  np->parent=curproc;
  if(curproc->hasthread == 1){
	np->mother = curproc;	
    //cprintf("//\n");
  }
  else{
	np->mother = curproc->mother;
  }

  //copy parent's file descriptor
  for(i = 0; i < NOFILE; i++) 
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);
  
  safestrcpy(np->name, curproc->name,sizeof(curproc->name));


  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);
  //cprintf("[Create] DONE! pid : %d parent ; %d\n",np->pid,np->parent->pid);
  return 0;

 bad:
  return -1;

}

//thread_exit is syscall
//method to terminate the thread in it.
//we call thread_exit fun at the end of a thread routine.
void
thread_exit(void *retval){
  //cprintf("entry exit");
  struct proc *curproc = myproc();
  struct proc *p;

  if(curproc == initproc)
    panic("init exiting");

  begin_op();
  iput(curproc->cwd);
  end_op();

  curproc->cwd = 0;

  acquire(&ptable.lock);

  

  // Pass abandoned children to init.
  //# We have to modify this part when we consider interaction with syscall.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
	  p->parent = initproc;
	  if(p->state == ZOMBIE)
		wakeup1(initproc);
    }
  }
  //#
  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  curproc->ret_val = retval;

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  sched();
  panic("zombie exit");
} 

//thread_join is syscall
//method to wait for the thread specified by the argument to terminate.
//If that thread has already terminated, then this returns immediately.
//In this process, we have to clean up the resources including page_table,
//allocated memories and stacks.
int
thread_join(thread_t thread, void **retval){
  //cprintf("entr join\n");
  struct proc *p;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
	  //cprintf("entr join for \n");
      if(p->parent != curproc || p->pid != (int)thread)
        continue;
      if(p->state == ZOMBIE){
        // Found one.
        *retval = p->ret_val;
        kfree(p->kstack);
        p->kstack = 0;
        p->sz = curproc->sz;
	    curproc->ex_thread[p->thread_num]=0;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        p->tick = 0;
        p->MLFQ_lv = HIGH;
        p->proc_mode = MLFQ;
        p->CPU_SHARE = 0;
        p->PASS = 0;
        p->ret_val = 0;
	    p->thread_num = -1;
	    p->isthread = 0;
        p->mother = initproc;
        release(&ptable.lock);
        return 0;
      }
    }
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }

}


// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;
 
  acquire(&ptable.lock); 
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->pid == pid){
        p->killed = 1;
        // Wake process from sleep if necessary.
        if(p->state == SLEEPING)
          p->state = RUNNABLE;
        release(&ptable.lock);
        return 0;
	  }
	}
  
  release(&ptable.lock);
  return -1;
}
int
kill_lwpgroup(int pid,struct proc *np){
  struct proc *p;

  acquire(&ptable.lock);
  if(np->isthread == 1){
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->isthread == 1 && p->mother->pid == pid && p->pid != np->pid ){
		p->killed = 1; 
		// Wake process from sleep if necessary.
		if(p->state == SLEEPING)
          p->state = RUNNABLE;
      }    
    }
    

    release(&ptable.lock);
    return 0;
  }
     

  release(&ptable.lock);
  return -1;


}
// 2022-06-01 implement basic operation for semaphore(xem)
// initialize the semaphore structure. Initial value should be 1.
int 
xem_init(xem_t *semaphore){
  //semaphore = malloc(sizeof(xem_t));
  semaphore-> value = 1;
  return 0;
}
// we will use xchg function which is atomic version of TAS function
int 
TAS(int*ptr, int new){
  int old = *ptr;
  *ptr = new;
  return old;
}
int 
xem_wait(xem_t *semaphore){
  while(1){
   //	acquire(&atomic_lock);
   //	if(TAS(&semaphore->value,0)==1){
	if(xchg(&semaphore->value,0)==1){
	 // release(&atomic_lock);
	  return 0;	
	}
   // release(&atomic_lock);
	yield();
  }

  return -1;
}

int 
xem_unlock(xem_t *semaphore){
	while(1){
	  //acquire(&atomic_lock);
	  //if(TAS(&semaphore->value,1)==0){
	  if(xchg(&semaphore->value,1)==0){
		//release(&atomic_lock);
		return 0;
	  }
	  yield();
	  //release(&atomic_lock);
	}

  return -1;
}
// 2022-06-01 implement basic operation for reader-writer lock(rwlock_t)
int
rwlock_init(rwlock_t *rw){
  rw->readers = 0;
  xem_init(&rw->lock);
  xem_init(&rw->writelock);
  return 0;
}

int
rwlock_acquire_readlock(rwlock_t *rw){
  xem_wait(&rw->lock);
  rw->readers++;
  if(rw->readers == 1){
	xem_wait(&rw->writelock);
  }
  xem_unlock(&rw->lock);
  return 0;
}
int
rwlock_release_readlock(rwlock_t *rw){
  xem_wait(&rw->lock);
  rw->readers--;
  if(rw->readers == 0){
    xem_unlock(&rw->writelock);
  }
  xem_unlock(&rw->lock);
  return 0;
}

int
rwlock_acquire_writelock(rwlock_t *rw){
  xem_wait(&rw->writelock);
  return 0;
}

int
rwlock_release_writelock(rwlock_t *rw){
  xem_unlock(&rw->writelock);
  return 0;
}



//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
