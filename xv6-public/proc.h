// Per-CPU state
struct cpu {
  uchar apicid;                // Local APIC ID
  struct context *scheduler;   // swtch() here to enter scheduler
  struct taskstate ts;         // Used by x86 to find stack for interrupt
  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  struct proc *proc;           // The process running on this cpu or null
};

extern struct cpu cpus[NCPU];
extern int ncpu;

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint edi;
  uint esi;
  uint ebx;
  uint ebp;
  uint eip;
};

/* 2022.03.31 MLFQ_level */
enum MLFQ_level { LOW = 0, MID = 1, HIGH = 2};       
// 2 is highest, 1 is middle ,0 is lowest level

/*2022.04.07 Process_mode*/
enum Proc_mode { MLFQ = 0, Stride = 1};
// MLFQ is default

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  int pid;                     // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)

/// 2022.03.31 adding MLFQ state for implementing MLFQ scheduling
  int tick;                    // tick for MLFQ scheduling
  enum MLFQ_level MLFQ_lv;     // Process MLFQ level

/// 2022.04.07 adding Stride state for combining Stride 
  enum Proc_mode proc_mode;    // proc  mode wheather MLFQ or Stride
  int CPU_SHARE;			   // CPU share for stride scheduling
  double PASS;				   // cpu time the proc use in stride mode
/// 2022.05.15 info about childthread for mother process
  uint sz_thread[NTHREAD];       // info about kstack_location
  uint ex_thread[NTHREAD];       // info about cthread exit (1 exit,0 not)
  int hasthread;                 // 1 is true, 0 is false
  int thread_num;                // thread_index

/// 2022.05.14 adding thread state 
  int isthread;                // 1 is true, 0 is false.
  void *ret_val;               // return value by tread.
  int exiting;				   // 1 is true, 0 is false.

// 2022.05.23 adding more info about threads
  struct proc *mother;         // mother process for thread
  int CPU_share_Stride; // Sum of cpu_share of each stride_mode thread
  double PASS_MLFQ;        // used cpu_time for MLFQ scheduling thread
  double PASS_Stride;      // used cpu_time for Stride scheduling thread
  int max_stride_thread_pass; //maximum pass in all stride threads
  int total_ticks_MLFQ;
  int CPU_S;
  int m_tick;                 // mother's tick
  enum MLFQ_level m_MLFQ_lv;     // Process MLFQ level


};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
