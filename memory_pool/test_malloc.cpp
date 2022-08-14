

#include "text_malloc.h"



//主要用来测试时间
int main()
{

    //和时间有关的类型

    clock_t start,end;
    start = clock();
    for(int i = 0; i< 5000000;++i)
    {
        A* pa = new A();
    }
    end = clock();

    std::cout<<"申请分配的内存次数为："<<A::m_iCount<<std::endl;
    std::cout<<"实际malloc的次数为:"<<A::m_iMallocCount<<std::endl;
    std::cout<<"用时(毫秒):"<<end-start<<std::endl;


    return 0;
}
