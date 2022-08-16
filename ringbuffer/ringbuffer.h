#ifndef _RINGBUFFER_H__
#define _RINGBUFFER_H__

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define min(lth , rth) ((lth) < (rth) ? (lth): (rth))

typedef struct
{
    uint8_t* buf;       //buffer
    uint32_t size;      //大小
    uint32_t read_pos;  //读的位置,也是循环的缓冲区，从0到2^32-1
    uint32_t write_pos; //写的位置
}ringbuffer_t;

//接口总结：1.初始化,2.判断是否为空,3.判断满,4.释放内存
//基础接口
void rb_init(ringbuffer_t* t ,uint32_t sz);
int32_t rb_is_empty(ringbuffer_t* t);
int32_t  rb_is_full(ringbuffer_t* t);
void rb_free(ringbuffer_t* t);
uint32_t rb_length(ringbuffer_t* t);
uint32_t rb_reamin(ringbuffer_t* t);

//读写数据接口
uint32_t rb_write(ringbuffer_t* dst_t,void* src_buf,uint32_t src_sz);
uint32_t rb_read(ringbuffer_t* src_t,void* dst_buf,uint32_t dst_sz);

//转换成2的n次幂
static inline uint32_t roundup_power_of_two(uint32_t sz)
{
    if(sz < 2) return sz = 2;

    //5 -> 8   101 -> 1000 
    int i = 0;
    for(i = 0;sz != 0;++i)
        sz>>1;
    return 1 << i;
}

//判断是否是2的n次幂
static inline int32_t  is_power_of_two(uint32_t sz)
{
    if(sz < 2) return -1;

    //m%n = m & (n-1)
    //m%n = m - n * floor(m/n)
    return (sz & (sz - 1)) == 0;
}


#endif
