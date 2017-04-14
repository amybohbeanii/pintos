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
