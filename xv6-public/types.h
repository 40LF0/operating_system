typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
//2022-05-13 add thread_t type
typedef uint thread_t;
//2022-06-01 add xem_t type
typedef struct
{
  uint value;
}xem_t;

//2022-06-03 add rwlock_t type
typedef struct
{
  xem_t lock;
  xem_t writelock;
  int readers;
}rwlock_t;
