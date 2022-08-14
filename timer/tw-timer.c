#include <stdio.h>
#include <unistd.h>

#include <pthread.h>
#include <time.h>
#include <stdlib.h>
#include "timewheel.h"

struct context {
	int quit;
    int thread;
};

struct thread_param {
	struct context *ctx;
	int id;
};

static struct context ctx = {0};

void do_timer(timer_node_t *node) {
    printf("timer expired:%d - thread-id:%d\n", node->expire, node->id);
}

//线程执行函数
void* thread_worker(void *p) {
	struct thread_param *tp = p;
	int id = tp->id;                            //表示是第几个线程
    struct context *ctx = tp->ctx;              //
	while (!ctx->quit) {
        int expire = rand() % 200;              //随机过期时间
        add_timer(expire, do_timer, id);        //把随机出来的过期时间作为节点加入定时器
        usleep(expire*(10-1)*1000);
    }
    printf("thread_worker:%d exit!\n", id);
    return NULL;
}

void do_quit(timer_node_t * node) {
    ctx.quit = 1;
}

int main() {
    srand(time(NULL));
    ctx.thread = 8;
    //1、创建8个线程
    pthread_t pid[ctx.thread];

    //2、初始化定时器
    init_timer();

    //3.增加定时器节点，延迟6s后改变推退出标签
    add_timer(6000, do_quit, 100);

    //4、创建8个线程管理器管理线程，这个几个线程负责往链表数组添加节点
    struct thread_param task_thread_p[ctx.thread];
    int i;
    for (i = 0; i < ctx.thread; i++) {
        task_thread_p[i].id = i;
        task_thread_p[i].ctx = &ctx;
        if (pthread_create(&pid[i], NULL, thread_worker, &task_thread_p[i])) {
            fprintf(stderr, "create thread failed\n");
            exit(1);
        }
    }

    //5、循环遍历数组，执行要过期节点和将高维要过期节点放到低维数组链表里面
    while (!ctx.quit) {
        expire_timer();
        usleep(2500);
    }

    //6、回收线程
    clear_timer();
    for (i = 0; i < ctx.thread; i++) {
		pthread_join(pid[i], NULL);
    }
    printf("all thread is closed\n");
    return 0;
}

// gcc tw-timer.c timewheel.c -o tw -I./ -lpthread 
