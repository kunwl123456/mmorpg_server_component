

#include <iostream>
#include <ctime>
typedef long clock_t;




class CAllocator
{
public:
    CAllocator(){}

    bool init();
    void*   allocate(size_t size);
    void    deallocate(void* phead);   


private:
    //嵌入式指针，写在类内，只在类内使用
    struct obj
    {
        struct obj* next;
    };
    obj* m_FreePosi = nullptr;                           //总是指向一块可以分配出去的内存的首地址
    int m_sTrunkCount=5;                       //一次分配多少倍该类的内存


};


#define DECLARE_POOL_ALLOC() \
public:\
    static void* operator new(size_t size) \
    {\
        return allocate.allocate(size);\
    }\

    static void operator delete(void* pHead)\
    {\
        return allocate.deallocate(pHead);\
    }\
    static CAllocator allocate;

#define IMPLEMENT_POOL_ALLOC(classname)\
CAllocator classname::allocate;

class A
{
public:
    DECLARE_POOL_ALLOC()
public:
    //这里是确保能使用嵌入式指针，至少分配4字节内存，这用了8字节内存
    int32_t m_i;
    int32_t m_j;

public:


};

IMPLEMENT_POOL_ALLOC(A)











