#include "ringbuffer.h"

int32_t rb_is_empty(ringbuffer_t* t)
{
    return t->read_pos == t->write_pos;
}

int32_t  rb_is_full(ringbuffer_t* t)
{
    //优化前
    //return (t->read_pos+t->size)%t->size == t->write_pos;

    //优化前提：
    //1）缓冲区的大小必须是2的n次幂 (UInt3_t大小为2^32-1,范围是0到2^32 -1)

    return t->size == t->write_pos - t->read_pos;
}


void rb_init(ringbuffer_t* t ,uint32_t sz)
{
    t->buf = (uint8_t*)malloc(sz*sizeof(uint8_t*));
    t->size = sz;
    t->read_pos = t->write_pos = UINT32_MAX -2;
    return;
}

void rb_free(ringbuffer_t* t)
{
    if(t->buf)
    {
        free(t->buf);
        t->buf = NULL;
    }

    t->read_pos = t->write_pos = t->size = 0;

    return;
}

uint32_t rb_length(ringbuffer_t* t)
{
    return t->write_pos - t->read_pos;
}

uint32_t rb_reamin(ringbuffer_t* t)
{
    return t->size - rb_length(t);
}

uint32_t rb_write(ringbuffer_t* dst_t,void* src_buf,uint32_t src_sz)
{
    if(src_sz > rb_reamin(dst_t))
        return -1;
    
    //dst_t->write_pos & (dst_t->size - 1)  把写坐标弄到这个圆环的范围之内
    //看哪个剩余空间最小
    //安全的拷贝
    uint32_t i = min(src_sz,dst_t->size - dst_t->write_pos & (dst_t->size - 1));

    //先拷贝后面的,再拷贝前面的
    memcpy(dst_t->buf + (dst_t->write_pos & (dst_t->size - 1)), src_buf,i);
    memcpy(dst_t->buf,src_buf + i,src_sz - i);//这里src_sz - i是负数是不会拷贝的

    dst_t->write_pos += src_sz;
    return src_sz;
}

uint32_t rb_read(ringbuffer_t* src_t,void* dst_buf,uint32_t dst_sz)
{
    if(rb_is_empty(src_t))
        return -1;
    uint32_t i = 0;
    dst_sz = min(dst_sz,rb_length(src_t) ); 
    //rb_length(src_t)为实际的大小
    //dst_sz为预期的大小

    i = min(dst_sz,(src_t->size - src_t->read_pos & (src_t->size - 1)));
    //src_t->read_pos & (src_t->size - 1))表示read_pos在size的位置
    //src_t->size - src_t->read_pos & (src_t->size - 1)表示求出右边空余位置的大小

    memcpy(dst_buf,src_t->buf + (src_t->read_pos & (src_t->size - 1)),i);
    memcpy(dst_buf + i,src_t->buf,dst_sz - i);

    src_t->read_pos +=dst_sz;

    return dst_sz;

}

int main()
{
    ringbuffer_t rb;
    rb_init(&rb,9);

    uint32_t put1 = rb_write(&rb,(void*)("mark"),4);
    printf("put1 = %u,wr = %u,rd = %u,length = %u\n",put1,rb.write_pos,rb.read_pos,rb_length(&rb));

    return 0;
}


