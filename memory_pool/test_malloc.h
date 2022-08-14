#include <iostream>
#include <ctime>
typedef long clock_t;


class A
{
public:
    static      void*   operator    new(size_t size);
    static      void    operator    delete(void* phead);   
    static      int     m_iCount;                           //用于统计分配次数统计，每new一次
    static      int     m_iMallocCount;                     //用于统计malloc次数，每malloc一次加1

private:
    A* next;
    static A* m_FreePosi;                                   //总是指向一块可以分配出去的内存的首地址
    static int m_sTrunkCount;                               //一次分配多少倍该类的内存
};

void* A::operator new(size_t size)
{
    //不再用传统方式实现，而是用内存池实现
    
    //传统方式
    //A* ppoint = (A*)malloc(size);
    //return ppoint;

    //1.初始化内存池
    A* tmplink;
    if(m_FreePosi == nullptr)
    {
        //2.为空，我们要申请一大块内存
        size_t realsize = m_sTrunkCount*size;               //要申请m_sTrunkCount这么多的内存
        m_FreePosi = reinterpret_cast<A*>(new char[realsize]);//传统new，调用底层传统malloc

        tmplink = m_FreePosi;

        //3.把分配的内存链接起来
        for(;tmplink !=&m_FreePosi[m_sTrunkCount - 1]; ++tmplink)
        {
            tmplink->next = tmplink + 1;
        }
        tmplink->next = nullptr;
        ++m_iMallocCount;

    }

    //4.归位
    tmplink = m_FreePosi;
    m_FreePosi = m_FreePosi->next;//m_FreePosi总是指向能分配内存的下一块首地址
    ++m_iCount;
    return tmplink;
}

void  A::operator delete(void* phead)
{
    //1.传统
    //free(phead);

    //2.内存池做法
    (static_cast<A*>(phead)->next) = m_FreePosi;
    m_FreePosi = static_cast<A*>(phead);

}

int A::m_iCount = 0;
int A::m_iMallocCount = 0;
A* A::m_FreePosi = nullptr;
int A::m_sTrunkCount = 500;



