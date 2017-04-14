            +----------------------+
            |   Software Systems   |
            |  Project 1: Threads  |
            |   DESIGN DOCUMENT    | 
            +----------------------+
## ---- GROUP ----
 * Amy Wu <jiaxuan@brandeis.edu>
 * Bonnie
 * Apurva
 * Andrew 

## ---- PRELIMINARIES ----
>> If you have any preliminary comments on your submission, notes for
>> the TA, please give them here.

>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation,
>> course text, and lecture notes.
None.

                   Alarm Clock
                   ===========
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

##### files changed and purpose
 * timer.c: to reimplement timer_sleep() in devices/timer.c to avoid busy waiting. Compares threads based on wake up times and insert into a wait list. 
##### functions
* Reimplement timer_sleep() in devices/timer.c to avoid busy waiting
* alarm clock implementation is not needed for later projects.

/*struct to store a list of waiting/sleeping threads*/
```
static struct list wait_list;
list_init(&wait_list);
```
/*function to compare two threads based on their wakeup times*/
```
static bool compare_threads_by_wakeup_time(const struct list_elem *a_,const struct list_elem *b_,void *aux UNUSED){
	const struct thread *a = list_entry(a_,struct thread,timer_elem);
	const struct thread *b = list_entry(b_,struct thread,timer_elem);
	return a->wakeup_time<b->wakeup_time;
}	
```
/*function to schedule wakeup time and add thread to waitlist*/
```
struct thread *t = thread_current();
t->wakeup_time = timer_ticks()+ticks;
ASSERT(intr_get_level() == INTR_ON);
intr_disable();
list_insert_ordered(&wait_list,&t->timer_elem,compare_threads_by_wakeup_time,NULL);
intr_enable();
sema_down(&t->timer_sema);
```
/*function to handle timer interrupts*/
```
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  thread_tick ();
  
  /*new*/
  while(!list_empty(&wait_list)){
    struct thread *t = list_entry(list_front(&wait_list),struct thread,timer_elem);
    if(ticks<t->wakeup_time)
      break;
    sema_up(&t->timer_sema);
    thread_yield_to_higher_priority();
    list_pop_front(&wait_list);
  }
}
```
 * next file
```
/* Struct to store a sleeping thread along with its
   data indicating how much time it has left to sleep. */
typedef struct thread_sleeping {
  int64_t time_slept;
  int64_t total_time;
  struct thread *thread_ptr;
  struct list_elem elem;
} thread_sleeping, *thread_sleeping_ptr;
```
```
/* List of threads that are currently sleeping */
static thread_sleeping sleeping_threads;
```
```
/* Sleeps for approximately TICKS timer ticks.  Interrupts must
   be turned on. */
void
timer_sleep (int64_t ticks) 
{
  int64_t start = timer_ticks ();
	
  struct list *sleeping_list = get_sleeping_list();
  intr_set_level(INTR_ON);
  ASSERT (intr_get_level () == INTR_ON);
  
  struct thread *cur = thread_current();

  cur->sleep_start = start;
  cur->sleep_total = ticks;
  list_push_back (sleeping_list, &cur->sleeping_elem);
  intr_set_level(INTR_OFF);
  thread_block();
  intr_set_level(INTR_ON);
}
```
```
static void update_sleeping_threads(void)
{
  struct list_elem *e;
  struct list *sleeping_list = get_sleeping_list();
  
  for (e = list_begin (sleeping_list); e != list_end (sleeping_list);
       e = list_next (e))
    {
       struct thread *t = list_entry (e, struct thread, sleeping_elem);
       
       if(timer_elapsed(t->sleep_start) >= t->sleep_total)
	   {
	     thread_unblock(t);
	     list_remove(e);
	   }
    }
}

/* Timer interrupt handler. */
static void
timer_interrupt (struct intr_frame *args UNUSED)
{
  ticks++;
  
  // Update all sleeping threads
  update_sleeping_threads();
  
  thread_tick ();
  
}
```

A "latch" is a new synchronization primitive. Acquires block
until the first release. Afterward, all ongoing and future
acquires pass immediately.

/* Latch. */
struct latch
{
bool released;
struct lock monitor_lock;
struct condition rel_cond;
};
/* Released yet? */
/* Monitor lock. */
/* Signaled when released. */
Added to struct thread:
/* Members for implementing thread_join(). */
struct latch ready_to_die;
/* Release when thread about to die. */
struct semaphore can_die;
/* Up when thread allowed to die. */
struct list children;
/* List of child threads. */
list_elem children_elem;
/* Element of ‘children’ list. */

### ---- ALGORITHMS ----
>> Briefly describe your implementation of thread_join() and how it
>> interacts with thread termination.

thread_join() finds the joined child on the thread’s list of
children and waits for the child to exit by acquiring the child’s
ready_to_die latch. When thread_exit() is called, the thread
releases its ready_to_die latch, allowing the parent to continue.

### ---- SYNCHRONIZATION ----
>> Consider parent thread P with child thread C. How do you ensure
>> proper synchronization and avoid race conditions when P calls wait(C)
>> before C exits? After C exits? How do you ensure that all resources
>> are freed in each case? How about when P terminates without waiting,
>> before C exits? After C exits? Are there any special cases?

C waits in thread_exit() for P to die before it finishes its own
exit, using the can_die semaphore "down"ed by C and "up"ed by P as
it exits. Regardless of whether whether C has terminated, there
is no race on wait(C), because C waits for P’s permission before
it frees itself.
Regardless of whether P waits for C, P still "up"s C’s can_die
semaphore when P dies, so C will always be freed. (However,
freeing C’s resources is delayed until P’s death.)
The initial thread is a special case because it has no parent to
wait for it or to "up" its can_die semaphore. Therefore, its
can_die semaphore is initialized to 1.

### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.

This design has the advantage of simplicity. Encapsulating most
of the synchronization logic into a new "latch" structure
abstracts what little complexity there is into a separate layer,
making the design easier to reason about. Also, all the new data
members are in ‘struct thread’, with no need for any extra dynamic
allocation, etc., that would require extra management code.
On the other hand, this design is wasteful in that a child thread
cannot free itself before its parent has terminated. A parent
thread that creates a large number of short-lived child threads
could unnecessarily exhaust kernel memory. This is probably
acceptable for implementing kernel threads, but it may be a bad
idea for use with user processes because of the larger number of
resources that user processes tend to own.

                   Priority Scheduling
                   ===================
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

#### functions
* threads/thread.c
* void thread_set_priority (int new_priority)
* int thread_get_priority (void)
* not used in later projects

### ---- ALGORITHMS ----
>> Briefly describe your implementation of thread_join() and how it
>> interacts with thread termination.

thread_join() finds the joined child on the thread’s list of
children and waits for the child to exit by acquiring the child’s
ready_to_die latch. When thread_exit() is called, the thread
releases its ready_to_die latch, allowing the parent to continue.

### ---- SYNCHRONIZATION ----
>> Consider parent thread P with child thread C. How do you ensure
>> proper synchronization and avoid race conditions when P calls wait(C)
>> before C exits? After C exits? How do you ensure that all resources
>> are freed in each case? How about when P terminates without waiting,
>> before C exits? After C exits? Are there any special cases?

C waits in thread_exit() for P to die before it finishes its own
exit, using the can_die semaphore "down"ed by C and "up"ed by P as
it exits. Regardless of whether whether C has terminated, there
is no race on wait(C), because C waits for P’s permission before
it frees itself.
Regardless of whether P waits for C, P still "up"s C’s can_die
semaphore when P dies, so C will always be freed. (However,
freeing C’s resources is delayed until P’s death.)
The initial thread is a special case because it has no parent to
wait for it or to "up" its can_die semaphore. Therefore, its
can_die semaphore is initialized to 1.

### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.

This design has the advantage of simplicity. Encapsulating most
of the synchronization logic into a new "latch" structure
abstracts what little complexity there is into a separate layer,
making the design easier to reason about. Also, all the new data
members are in ‘struct thread’, with no need for any extra dynamic
allocation, etc., that would require extra management code.
On the other hand, this design is wasteful in that a child thread
cannot free itself before its parent has terminated. A parent
thread that creates a large number of short-lived child threads
could unnecessarily exhaust kernel memory. This is probably
acceptable for implementing kernel threads, but it may be a bad
idea for use with user processes because of the larger number of
resources that user processes tend to own.

                        Advanced Scheduler
                        ==================
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

#### Functions
* not used in later projects

### ---- ALGORITHMS ----
>> Briefly describe your implementation of thread_join() and how it
>> interacts with thread termination.

thread_join() finds the joined child on the thread’s list of
children and waits for the child to exit by acquiring the child’s
ready_to_die latch. When thread_exit() is called, the thread
releases its ready_to_die latch, allowing the parent to continue.

### ---- SYNCHRONIZATION ----
>> Consider parent thread P with child thread C. How do you ensure
>> proper synchronization and avoid race conditions when P calls wait(C)
>> before C exits? After C exits? How do you ensure that all resources
>> are freed in each case? How about when P terminates without waiting,
>> before C exits? After C exits? Are there any special cases?

C waits in thread_exit() for P to die before it finishes its own
exit, using the can_die semaphore "down"ed by C and "up"ed by P as
it exits. Regardless of whether whether C has terminated, there
is no race on wait(C), because C waits for P’s permission before
it frees itself.
Regardless of whether P waits for C, P still "up"s C’s can_die
semaphore when P dies, so C will always be freed. (However,
freeing C’s resources is delayed until P’s death.)
The initial thread is a special case because it has no parent to
wait for it or to "up" its can_die semaphore. Therefore, its
can_die semaphore is initialized to 1.

### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.

This design has the advantage of simplicity. Encapsulating most
of the synchronization logic into a new "latch" structure
abstracts what little complexity there is into a separate layer,
making the design easier to reason about. Also, all the new data
members are in ‘struct thread’, with no need for any extra dynamic
allocation, etc., that would require extra management code.
On the other hand, this design is wasteful in that a child thread
cannot free itself before its parent has terminated. A parent
thread that creates a large number of short-lived child threads
could unnecessarily exhaust kernel memory. This is probably
acceptable for implementing kernel threads, but it may be a bad
idea for use with user processes because of the larger number of
resources that user processes tend to own.


