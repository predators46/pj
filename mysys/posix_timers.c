/* Copyright (c) 2014, 2015, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */


#include "my_global.h"
#include "my_thread.h"      /* my_thread_init, my_thread_end */
#include "my_timer.h"       /* my_timer_t */

#include <signal.h>
#include <string.h>         /* memset */

typedef union mysigval {
        int sival_int;
        void *sival_ptr;
} mysigval;

/**
  Timer expiration notification thread.

 @param  arg   Event info.
*/

static void
timer_notify_thread_func(mysigval arg) {
  my_timer_t *timer = (my_timer_t *)(arg.sival_ptr);
  timer->notify_function(timer);
}



int
my_timer_initialize()
{
  return 0;
}



void
my_timer_deinitialize(void)
{

}


/**
  Create a timer object.

  @param  timer   Location where the timer ID is returned.

  @return On success, 0.
          On error, -1 is returned, and errno is set to indicate the error.
*/

int
my_timer_create(my_timer_t *timer)
{
  struct sigevent sigev;

  memset(&sigev, 0, sizeof(sigev));

  sigev.sigev_notify= SIGEV_THREAD;
  sigev.sigev_value.sival_ptr= timer;
  sigev.sigev_notify_function= &timer_notify_thread_func;

#ifdef __sun // CLOCK_MONOTONIC not supported on Solaris even if it compiles.
  return timer_create(CLOCK_REALTIME, &sigev, &timer->id);
#else
  return timer_create(CLOCK_MONOTONIC, &sigev, &timer->id);
#endif
}


/**
  Set the time until the next expiration of the timer.

  @param  timer   Timer object.
  @param  time    Amount of time (in milliseconds) before the timer expires.

  @return On success, 0.
          On error, -1 is returned, and errno is set to indicate the error.
*/

int
my_timer_set(my_timer_t *timer, unsigned long time)
{
  const struct itimerspec spec= {
    .it_interval= {.tv_sec= 0, .tv_nsec= 0},
    .it_value= {.tv_sec= time / 1000,
                .tv_nsec= (time % 1000) * 1000000}
  };

  return timer_settime(timer->id, 0, &spec, NULL);
}


/**
  Cancel the timer.

  @param  timer   Timer object.
  @param  state   The state of the timer at the time of cancellation, either
                  signaled (false) or nonsignaled (true).

  @return On success, 0.
          On error, -1 is returned, and errno is set to indicate the error.
*/

int
my_timer_cancel(my_timer_t *timer, int *state)
{
  int status;
  struct itimerspec old_spec;

  /* A zeroed initial expiration value disarms the timer. */
  const struct timespec zero_time= { .tv_sec= 0, .tv_nsec= 0 };
  const struct itimerspec zero_spec= { .it_value= zero_time };

  /*
    timer_settime returns the amount of time before the timer
    would have expired or zero if the timer was disarmed.
  */
  if (! (status= timer_settime(timer->id, 0, &zero_spec, &old_spec)))
    *state= (old_spec.it_value.tv_sec || old_spec.it_value.tv_nsec);

  return status;
}


/**
  Delete a timer object.

  @param  timer   Timer object.
*/

void
my_timer_delete(my_timer_t *timer)
{
  timer_delete(timer->id);
}

