# [PintOS Project-3]  Alarm clock, priority scheduling



### Alarm clock

**문제 상황**

`timer_sleep()` 함수는 busy waiting 방식으로 process를 sleep 시키는데 이 방식은 run state, ready state를 반복적으로 돌게 한다. 따라서 매우 비효율적이다. 

**비효율을 피하는 방법**

1. Tick을 체크한 후 완료되지 않았다면 block state로 변경한다.
2. 이런 스레드를 관리하기 위해서는 해당 스테이트를 달아놓는 새로운 queue를 만들어야 함
3. 큐에 스레드 꽂을 때에는 일어나는 시간도 저장해놓는다.
4. 시간이 다 되면 스레드를 깨워서 ready queue(= ready_list)에 달아 놓는다.

**스레드가 block된 상태에서는 어떻게 시간이 다 됬는지 알 수 있을까?**

1. 매 tick마다 `timer_interrupt()`가 호출된다.

2. 위 함수에서는 일어나야 되는 스레드를 찾고, (큐에 꽂을 때 일어날 시간 저장해놓은 것을 통해서) ready queue에 넣는다.

**개선을 위해 수정한 함수**

```c
void timer_sleep (int64_t ticks);
void thread_yield (void);
void thread_init (void);
void thread_sleep (int64_t ticks);
bool value_less (const struct list_elem *a_, 
                 const struct list_elem *b_, 
                 void *aux UNUSED);
static void timer_interrupt (struct intr_frame *args UNUSED);
void thread_wakeup (void);
void thread_tick (void);

```

<br>

### Priority scheduling

**현재 상황**

핀토스는 지금까지 round-robin 방식으로 스케쥴 시켰다. ﻿다시말해서, `thread_yield()`, `thread_unblock()`이 호출되면 현재 스레드나 block된 스레드가 레디 큐 뒤로 들어갔는데, 이 때 우선순위를 고려하지 않았었다.

**해야 할 것**

﻿우선순위를 고려하는 스케쥴러를 짜는 것이다. ﻿새로운 스레드가 들어왔을 때 요놈이 현재 스레드보다 우선순위가 높으면 양보하는 것이다.

**배경 지식**

1. 스케쥴 우선순위는 `PRI_MIN` ~ `PRI_MAX` (0~63)의 범위를 가지며, 숫자가 높을수록 높은 우선순위이다.

2. 스케쥴 우선순위 기본값은 `PRI_DEFAULT` (31) 이다.

3. 맨 처음 스레드의 우선순위는 `thread_create()`를 통해 인자로 넘어간다.

**Aging이 필요하다.**

1. 위에 설명된 단순한 priority scheduler는 우선순위 낮은 프로세스의 starvation을 유발시킨다.

2. 그래서 프로세스가 레디 큐에 있는 동안 지난 시간에 비례해서 우선순위를 증가시켜줘야 한다.

**schedule 함수를 호출하는 함수들**

1. `thread_block()`에서 스레드를 블락 상태로 바꾸고 호출한다.

2. `thread_exit()`에서 스레드를 죽이고 호출

3. `thread_yield()`에서 스레드를 레디큐에 넣고, 레디 상태로 바꾸고 호출

   → **레디큐에 우선순위 고려해서 넣기**

4. `thread_sleep()`에서 스레드를 블락 상태로 바꾸고 호출

﻿**Preemption 구현**

스레드가 실행되고 있던 중, ready_list에 있던 스레드의 priority에 변화가 생길 수 있다. 만약 ready_list에 있던 스레드가 현재 돌고 있는 스레드보다 우선순위가 높아진다면, 지금 돌고 있는 스레드는 CPU를 양보해줘야 한다.

﻿그래서, ready_list의 스레드 우선순위가 바뀔 수 있는 경우는 무엇인가?

1. 새로운 스레드가 들어오는 경우 (들어온 스레드는 일단 ready_list에 들어감)

2. 현재 스레드의 priority를 임의로 바꾸는 경우

두 경우에 대해서 우선순위를 체크하고, 조건이 충족되면 preemption을 일으킨다. 


