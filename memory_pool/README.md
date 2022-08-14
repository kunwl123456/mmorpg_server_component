


memmory_pool analysis:

1）测试内存池对于速度的比较（时间对比,涉及文件test_malloc.cpp和.h）
①一次分配5个对象大小，分配100W次，耗时78767ms，也就是78s（for循环调用500次）
②一次分配500个对象大小，分配1W次，耗时53223ms，也就是53s（for循环调用5次）
③只用malloc分配500W次大小耗时187964ms，也就是187s（for循环调用500W次）
④总结：相差99W次，相差25s，每次分配相差耗时0.025ms,可以看出来用内存池不规定分配数量时其实不耗时，反而只用malloc一个个地分配对象内存就很耗时，大概有2到3倍耗时对比

2）仿造nginx页缓存做内存池
(1)涉及到的文件
ngx_page_pool.h
ngx_page_pool.cpp
(2)仿照ngx按照一页一页(4096字节)的分配内存的内存池设计
(3)关于如果一个节点剩下128字节的空间，若分配150个字节的空间，nginx是重试4，5次才改变当前指针
(4)API解释
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


3）模板做内存池








