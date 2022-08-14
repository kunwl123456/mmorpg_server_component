#include "spinlock.h"
#include "timewheel.h"
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

#if defined(__APPLE__)
#include <AvailabilityMacros.h>
#include <sys/time.h>
#include <mach/task.h>
#include <mach/mach.h>
#else
#include <time.h>
#endif

typedef struct link_list {
	timer_node_t head;   	//节点
	timer_node_t *tail;   	//后续节点
}link_list_t;

typedef struct timer {
	link_list_t near[TIME_NEAR];
	link_list_t t[4][TIME_LEVEL];
	struct spinlock lock;
	uint32_t time;					//10ms?
	uint64_t current;
	uint64_t current_point;  		//记录当前时间
}s_timer_t;

static s_timer_t * TI = NULL;     	//静态的定时器管理器

timer_node_t *
link_clear(link_list_t *list) {
	//返回当前列表的首个元素，然后将当前列表清空
	timer_node_t * ret = list->head.next;
	list->head.next = 0;
	list->tail = &(list->head);

	return ret;
}

//尾插节点到链表里面
void
link(link_list_t *list, timer_node_t *node) {
	list->tail->next = node;
	list->tail = node;
	node->next=0;
}

void
add_node(s_timer_t *T, timer_node_t *node) {
	uint32_t time=node->expire;
	uint32_t current_time=T->time;
	uint32_t msec = time - current_time;
	if (msec < TIME_NEAR) { //[0, 0x100)
		link(&T->near[time&TIME_NEAR_MASK],node);
	} else if (msec < (1 << (TIME_NEAR_SHIFT+TIME_LEVEL_SHIFT))) {//[0x100, 0x4000)
		link(&T->t[0][((time>>TIME_NEAR_SHIFT) & TIME_LEVEL_MASK)],node);	
	} else if (msec < (1 << (TIME_NEAR_SHIFT+2*TIME_LEVEL_SHIFT))) {//[0x4000, 0x100000)
		link(&T->t[1][((time>>(TIME_NEAR_SHIFT + TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);	
	} else if (msec < (1 << (TIME_NEAR_SHIFT+3*TIME_LEVEL_SHIFT))) {//[0x100000, 0x4000000)
		link(&T->t[2][((time>>(TIME_NEAR_SHIFT + 2*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);	
	} else {//[0x4000000, 0xffffffff]
		link(&T->t[3][((time>>(TIME_NEAR_SHIFT + 3*TIME_LEVEL_SHIFT)) & TIME_LEVEL_MASK)],node);	
	}
}

// 约定 time 为 10ms 为单位，若传进来 1000 则是10秒
timer_node_t*
add_timer(int time, handler_pt func, int threadid) {
	//初始化节点
	timer_node_t *node = (timer_node_t *)malloc(sizeof(*node));
	spinlock_lock(&TI->lock);
	node->expire = time+TI->time;  	//更新过期时间
	node->callback = func;			//回调函数
	node->id = threadid;			//线程索引，从1开始
	if (time <= 0) {				//--若不开延迟，即直接执行回调函数
		node->callback(node);		//执行回调函数
		spinlock_unlock(&TI->lock);	//解锁
		free(node);					//释放节点
		return NULL;
	}
	add_node(TI, node);				//增加节点
	spinlock_unlock(&TI->lock);		//解锁(对共享资源操作)
	return node;
}

void
move_list(s_timer_t *T, int level, int idx) {
	timer_node_t *current = link_clear(&T->t[level][idx]);
	while (current) {
		timer_node_t *temp=current->next;
		add_node(T,current);
		current=temp;
	}
}

//将t这个双重数组的节点按照过期时间重新挂到新的链表上去
void
timer_shift(s_timer_t *T) {
	int mask = TIME_NEAR;
	uint32_t ct = ++T->time;//将time自增
	if (ct == 0) {
		move_list(T, 3, 0);
	} else {
		// ct / 256
		uint32_t time = ct >> TIME_NEAR_SHIFT;
		int i=0;
		// ct % 256 == 0
		while ((ct & (mask-1))==0) {
			int idx=time & TIME_LEVEL_MASK;
			if (idx!=0) {
				move_list(T, i, idx);
				break;				
			}
			mask <<= TIME_LEVEL_SHIFT;
			time >>= TIME_LEVEL_SHIFT;
			++i;
		}
	}
}

//执行这个节点
void
dispatch_list(timer_node_t *current) {
	do {
		timer_node_t * temp = current;
		current=current->next;
        if (temp->cancel == 0)
            temp->callback(temp);
		free(temp);
	} while (current);
}

//执行定时器节点
void
timer_execute(s_timer_t *T) {
	int idx = T->time & TIME_NEAR_MASK;
	
	while (T->near[idx].head.next) {
		timer_node_t *current = link_clear(&T->near[idx]);
		spinlock_unlock(&T->lock);
		dispatch_list(current);
		spinlock_lock(&T->lock);
	}
}

void 
timer_update(s_timer_t *T) {
	spinlock_lock(&T->lock);
	timer_execute(T);				//执行最近要过期的节点
	timer_shift(T);					//将高级链表按照过期时间挂到near数组中去
	timer_execute(T);				//执行最近要过期的节点
	spinlock_unlock(&T->lock);
}

void
del_timer(timer_node_t *node) {
    node->cancel = 1;
}

s_timer_t *
timer_create_timer() {
	s_timer_t *r=(s_timer_t *)malloc(sizeof(s_timer_t));
	memset(r,0,sizeof(*r));
	int i = 0;
	int j = 0;
	for (i=0;i<TIME_NEAR;i++) {
		link_clear(&r->near[i]);
	}

	//将t这个双重数组清空
	for (i=0;i<4;i++) {
		for (j=0;j<TIME_LEVEL;j++) {
			link_clear(&r->t[i][j]);
		}
	}

	//初始化锁
	spinlock_init(&r->lock);

	//记录当前的定时器？？？
	r->current = 0;
	return r;
}

uint64_t
gettime() {
	uint64_t t;
#if !defined(__APPLE__) || defined(AVAILABLE_MAC_OS_X_VERSION_10_12_AND_LATER)
	struct timespec ti;
	clock_gettime(CLOCK_MONOTONIC, &ti);
	t = (uint64_t)ti.tv_sec * 100;
	t += ti.tv_nsec / 10000000;
#else
	struct timeval tv;
	gettimeofday(&tv, NULL);
	t = (uint64_t)tv.tv_sec * 100;
	t += tv.tv_usec / 10000;
#endif
	return t;
}

//
void
expire_timer(void) {
	uint64_t cp = gettime();
	if (cp != TI->current_point) {
		uint32_t diff = (uint32_t)(cp - TI->current_point);
		TI->current_point = cp;
		int i;
		for (i=0; i<diff; i++) {
			timer_update(TI);
		}
	}
}

void 
init_timer(void) {
	TI = timer_create_timer();
	TI->current_point = gettime();
}

void
clear_timer() {
	int i,j;
	for (i=0;i<TIME_NEAR;i++) {
		link_list_t * list = &TI->near[i];
		timer_node_t* current = list->head.next;
		while(current) {
			timer_node_t * temp = current;
			current = current->next;
			free(temp);
		}
		link_clear(&TI->near[i]);
	}
	for (i=0;i<4;i++) {
		for (j=0;j<TIME_LEVEL;j++) {
			link_list_t * list = &TI->t[i][j];
			timer_node_t* current = list->head.next;
			while (current) {
				timer_node_t * temp = current;
				current = current->next;
				free(temp);
			}
			link_clear(&TI->t[i][j]);
		}
	}
}

