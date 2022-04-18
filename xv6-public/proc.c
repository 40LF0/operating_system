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
int max_stride_proc_pass = 0;

// fun to check what schedule mode is
enum Proc_mode
sche_mode(void){
  if((num_Stride.CPU_share_Stride == 0)||(num_Stride.PASS_MLFQ <= num_Stride.PASS_Stride))
	return MLFQ;
  else
	return Stride;
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
    if(p->state == RUNNABLE && p->proc_mode == Stride){
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
	if(p->state == RUNNABLE && p->proc_mode == MLFQ){
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

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");
  // 2022-04-07 
  // revaluate CPU_share_Stride
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

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

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

    // Switch to chosen process.  It is the process's job
    // to release ptable.lock and then reacquire it
    // before jumping back to us.
    c->proc = p;
    switchuvm(p);
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
