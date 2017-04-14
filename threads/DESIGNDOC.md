            +----------------------+
            |   Software Systems   |
            |  Project 1: Threads  |
            |   DESIGN DOCUMENT    | 
            +----------------------+
## ---- GROUP ----
 * Amy Wu <jiaxuan@brandeis.edu>
 * Bonnie Ishiguro <Bonnie.Ishiguro@students.olin.edu>
 * Apurva Raman <Apurva.Raman@students.olin.edu>
 * Andrew Pan <Andrew.Pan@students.olin.edu>

## ---- PRELIMINARIES ----
>> If you have any preliminary comments on your submission, notes for
>> the TA, please give them here.

Alarm clock implementation is not needed for VM project. However, it can be useful for our VM implementation. Priority Scheduling is not needed for future projects. Advanced Scheduler is not used for future projects. 

>> Please cite any offline or online sources you consulted while
>> preparing your submission, other than the Pintos documentation,
>> course text, and lecture notes.

We used the provided Pintos solutions in order to understand Projects 1 and 2, so we can implement our own solution to Project 3: Virtual Memory quicker. One of our learning goals is to understand how threading works and we read chapter 9 in TOS in order to understand that.

                   Alarm Clock
                   ===========
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

|File changed | devices/timer.c |
|Function changed  | timer_sleep() reimplemented to avoid busy waiting. Compares threads based on wake up times and insert into a wait list.  |


```
/*struct to store a list of waiting/sleeping threads*/
static struct list wait_list;
list_init(&wait_list);
```

```
/*function to compare two threads based on their wakeup times*/
static bool compare_threads_by_wakeup_time(const struct list_elem *a_,const struct list_elem *b_,void *aux UNUSED){
	const struct thread *a = list_entry(a_,struct thread,timer_elem);
	const struct thread *b = list_entry(b_,struct thread,timer_elem);
	return a->wakeup_time<b->wakeup_time;
}	
```

```
/*function to schedule wakeup time and add thread to waitlist*/
void timer_sleep(int64_t ticks){
	struct thread *t = thread_current();
	t->wakeup_time = timer_ticks()+ticks;
	ASSERT(intr_get_level() == INTR_ON);
	intr_disable();
	list_insert_ordered(&wait_list,&t->timer_elem,compare_threads_by_wakeup_time,NULL);
	intr_enable();
	sema_down(&t->timer_sema);
}	
```

```
/*function to handle timer interrupts*/
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


### ---- ALGORITHMS ----
>> Briefly describe your implementation and how it
>> interacts with threading.

timer_sleep() avoids busy waiting by using semaphores.

### ---- SYNCHRONIZATION ----
>> How do you ensure proper synchronization?



### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.


                   Priority Scheduling
                   ===================
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

|File changed | threads/thread.c |
|Functions changed  | <ul><li>void thread_set_priority (int new_priority)</li><li>int thread_get_priority (void)</li></ul>   |


### ---- ALGORITHMS ----
>> Briefly describe your implementation and how it
>> interacts with threading.


### ---- SYNCHRONIZATION ----
>> How do you ensure proper synchronization?



### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.

                        Advanced Scheduler
                        ==================
### ---- DATA STRUCTURES ----
>> Copy here the declaration of each new or changed ‘struct’ or ‘struct’
>> member, global or static variable, ‘typedef’, or enumeration.
>> Identify the purpose of each in 25 words or less.

|File changed | threads/thread.h |


### ---- ALGORITHMS ----
>> Briefly describe your implementation and how it
>> interacts with threading.


### ---- SYNCHRONIZATION ----
>> How do you ensure proper synchronization?



### ---- RATIONALE ----
>> Critique your design, pointing out advantages and disadadvantages in
>> your design choices.

