

#include "page_pool.h"



bool CAllocator::init()
{


    return true;
Exit0:
    return false;
}

void* CAllocator::allocate(size_t size)
{
    //不再用传统方式实现，而是用内存池实现
    
    //传统方式
#ifdef MYMEMPOOL
    A* ppoint = (A*)malloc(size);
    return ppoint;
#endif
    //1.初始化内存池
    obj* tmplink = nullptr;
    if(m_FreePosi == nullptr)
    {
        //2.为空，我们要申请一大块内存
        size_t realsize = m_sTrunkCount*size;               //要申请m_sTrunkCount这么多的内存
#ifdef  NEWOLD //传统定位new方法
        m_FreePosi = reinterpret_cast<CPagePool*>(new char[realsize]);//传统new，调用底层传统malloc
#endif
        m_FreePosi = (obj*)malloc(realsize);

        tmplink = m_FreePosi;

        //3.把分配的内存链接起来(不等于末尾的内存块)
        for(int i = 0;i<m_sTrunkCount - 1;++i)
        {
            tmplink->next = (obj*)((char*)tmplink + size);
            tmplink = tmplink->next;
        }//end for

        tmplink->next = nullptr;
    }

    //4.归位
    tmplink = m_FreePosi;
    m_FreePosi = m_FreePosi->next;//m_FreePosi总是指向能分配内存的下一块首地址

    return tmplink;
}

void  CAllocator::deallocate(void* phead)
{
    //1.传统
#ifdef MYMEMPOOL
    free(phead);
    return;
#endif

    //2.内存池做法
    (static_cast<obj*>(phead)->next) = m_FreePosi;
    m_FreePosi = (obj*)(phead);

}


//主要用来测试时间
int main()
{
    A* mypa[100];
    for(int32_t i = 0;i < 15;++i)
    {
        mypa[i] = new A();
        printf("%p\n",mypa[i]);
    }

    for(int32_t i = 0;i < 15;++i)
    {
        delete mypa[i];
    }


    return 0;
}


















