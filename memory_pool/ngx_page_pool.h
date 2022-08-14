

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <fcntl.h>



#define MP_ALIGNMENT       		32
#define MP_PAGE_SIZE			4096
#define MP_MAX_ALLOC_FROM_POOL	(MP_PAGE_SIZE-1)

#define mp_align(n, alignment) (((n)+(alignment-1)) & ~(alignment-1))
#define mp_align_ptr(p, alignment) (void *)((((size_t)p)+(alignment-1)) & ~(alignment-1))

//分配4k以上的块
struct mp_large_s {
	struct mp_large_s *next;    //指向下一块内存
	void *alloc;                //指向内存块空间
};


//分配4K以下的块
//一个节点是一页
struct mp_node_s {

	unsigned char *last;        //指向当前这页的最后那个内存块
	unsigned char *end;         //指向用到哪个内存块的结尾
	
	struct mp_node_s *next;     //把一块一块的内存块组织起来
	size_t failed;              //引入尝试的失败次数
};


//内存池
struct mp_pool_s {

	size_t max;

	struct mp_node_s *current;      //指向当前小块分配的节点（一页内存块）
	struct mp_large_s *large;       //指向大块

	struct mp_node_s head[0];       //指向小块分配节点的头节点（一页内存块）

};

//全部API

//内存池的创建
struct mp_pool_s *mp_create_pool(size_t size);
//销毁内存池
void mp_destory_pool(struct mp_pool_s *pool);
//重置内存池，置空
void mp_reset_pool(struct my_pools_s* pool);
//分配小块内存
static void *mp_alloc_block(struct mp_pool_s *pool, size_t size);
//分配大块内存
static void *mp_alloc_large(struct mp_pool_s *pool, size_t size);
//对其分配内存
void *mp_memalign(struct mp_pool_s *pool, size_t size, size_t alignment);

//真正分配内存的入口
void *mp_calloc(struct mp_pool_s *pool, size_t size);
void *mp_alloc(struct mp_pool_s *pool, size_t size);
//释放
void mp_free(struct mp_pool_s *pool, void *p);



//内存池的创建
//size是节点的大小
struct mp_pool_s *mp_create_pool(size_t size) {

	struct mp_pool_s *p= nullptr;
	//分配4k以上内存用posix_memalign()这个函数分配
	//第一参数：返回的指针
	//第二参数：表示分配空间是以多大的空间对齐，这里宏定义为4k
	//第三参数：表示分配的大小size
	//sizeof（struct mp_pool_s）表示结构体放在节点的第一个块前面,放在里面是为了避免内存碎片
	int ret = posix_memalign((void **)&p, MP_ALIGNMENT, size + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s));
	if (ret) {          //如果分配失败的话
		return NULL;
	}
	
	p->max = (size < MP_MAX_ALLOC_FROM_POOL) ? size : MP_MAX_ALLOC_FROM_POOL;
	p->current = p->head;
	//p->current = p->head = p+1;
	p->large = NULL;

	p->head->last = (unsigned char *)p + sizeof(struct mp_pool_s) + sizeof(struct mp_node_s);
	p->head->end = p->head->last + size;    //end指向节点最末尾内存块的位置

	p->head->failed = 0;

	return p;

}

//销毁内存池
void mp_destory_pool(struct mp_pool_s *pool) {

	struct mp_node_s *h, *n;
	struct mp_large_s *l;

    //先释放大块的list
	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	h = pool->head->next;

    //再去释放node节点
	while (h) {
		n = h->next;
		free(h);
		h = n;
	}

	free(pool);

}

void mp_reset_pool(struct mp_pool_s *pool) {

	struct mp_node_s *h;
	struct mp_large_s *l;

	for (l = pool->large; l; l = l->next) {
		if (l->alloc) {
			free(l->alloc);
		}
	}

	pool->large = NULL;

	for (h = pool->head; h; h = h->next) {
		h->last = (unsigned char *)h + sizeof(struct mp_node_s);
	}

}

//分配小块内存
static void *mp_alloc_block(struct mp_pool_s *pool, size_t size) {

	unsigned char *m;
	struct mp_node_s *h = pool->head;
	size_t psize = (size_t)(h->end - (unsigned char *)h);
	
	int ret = posix_memalign((void **)&m, MP_ALIGNMENT, psize);
	if (ret) return NULL;

	struct mp_node_s *p, *new_node, *current;
	new_node = (struct mp_node_s*)m;

	new_node->end = m + psize;//end指向这个节点结尾的位置
	new_node->next = NULL;
	new_node->failed = 0;

	m += sizeof(struct mp_node_s);
	m = mp_align_ptr(m, MP_ALIGNMENT);
	new_node->last = m + size;

	current = pool->current;

    //循环遍历几次
    //针对一个块尝试多次分配，不行就移动到下一块（nginx）
	for (p = current; p->next; p = p->next) {
		if (p->failed++ > 4) {  //如果尝试4次，就遗弃这个块
		                        //为什么是4？经验值
			current = p->next;
		}
	}
	p->next = new_node;

	pool->current = current ? current : new_node;

	return m;

}

//分配大块内存
static void *mp_alloc_large(struct mp_pool_s *pool, size_t size) {

	void *p = malloc(size);
	if (p == NULL) return NULL;

	size_t n = 0;
	struct mp_large_s *large;
	for (large = pool->large; large; large = large->next) {
		if (large->alloc == NULL) { //如果不考虑大块回收的话，拿到尾节点就可以了
			large->alloc = p;
			return p;
		}
		if (n ++ > 3) break;
	}

	large = mp_alloc(pool, sizeof(struct mp_large_s));//把指向大块内存的结构体放在节点里面
	if (large == NULL) {
		free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}

void *mp_memalign(struct mp_pool_s *pool, size_t size, size_t alignment) {

	void *p;
	
	int ret = posix_memalign(&p, alignment, size);
	if (ret) {
		return NULL;
	}

	struct mp_large_s *large = mp_alloc(pool, sizeof(struct mp_large_s));
	if (large == NULL) {
		free(p);
		return NULL;
	}

	large->alloc = p;
	large->next = pool->large;
	pool->large = large;

	return p;
}



//分配内存
void *mp_alloc(struct mp_pool_s *pool, size_t size) {

    if( 0 >= size)
        return NULL;

	unsigned char *m;
	struct mp_node_s *p;

	if (size <= pool->max) {        //分配小块

		p = pool->current;          //找到分配的节点是哪个

		do {
			
			m = mp_align_ptr(p->last, MP_ALIGNMENT);
			if ((size_t)(p->end - m) >= size) {     //如果这个节点还有空间可以分配一个内存块，就分配一个内存块出去
				p->last = m + size;
				return m;
			}
			p = p->next;                            //如果这个节点没有一块内存的空间了，就找下一个节点
		} while (p);

		return mp_alloc_block(pool, size);//返回小块的内存
	}

	return mp_alloc_large(pool, size);  //分配大块，返回小块的内存
//关于如果一个节点剩下128字节的空间，若分配150个字节的空间，nginx是重试4，5次才改变当前指针
}


void *mp_nalloc(struct mp_pool_s *pool, size_t size) {

	unsigned char *m;
	struct mp_node_s *p;

	if (size <= pool->max) {
		p = pool->current;

		do {
			m = p->last;
			if ((size_t)(p->end - m) >= size) {
				p->last = m+size;
				return m;
			}
			p = p->next;
		} while (p);

		return mp_alloc_block(pool, size);
	}

	return mp_alloc_large(pool, size);
	
}

void *mp_calloc(struct mp_pool_s *pool, size_t size) {

	void *p = mp_alloc(pool, size);
	if (p) {
		memset(p, 0, size);
	}

	return p;
	
}

//释放
void mp_free(struct mp_pool_s *pool, void *p) {

	struct mp_large_s *l;
	for (l = pool->large; l; l = l->next) {
		if (p == l->alloc) {
			free(l->alloc);
			l->alloc = NULL;

			return ;
		}
	}
	
}







