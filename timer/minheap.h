#pragma once

#include <vector>
#include <map>
using namespace std;

typedef void (*TimerHandler) (struct TimerNode *node);

//定时器节点
struct TimerNode {
    int idx = 0;               //索引
    int id = 0;                //唯一ID
    unsigned int expire = 0;   //过期时间
    TimerHandler cb = NULL;    //回调函数
};

//定时器节点管理器
class MinHeapTimer {
public:
    MinHeapTimer() {
        _heap.clear();
        _map.clear();
    }
    static inline int Count() {
        return ++_count;
    }

    //增加定时器
    int AddTimer(uint32_t expire, TimerHandler cb);
    //删除定时器
    bool DelTimer(int id);
    //主循环处理定时器
    void ExpireTimer();

private:

    //堆排序比较函数
    inline bool _lessThan(int lhs, int rhs) {
        return _heap[lhs]->expire < _heap[rhs]->expire;
    }
    
    //堆排序相关函数

    //比较左右节点是否需要交换(小的过期时间放在左边)
    bool _shiftDown(int pos);

    //比较父节点和自己是否需要交换
    void _shiftUp(int pos);
    void _delNode(TimerNode *node);

private:
    vector<TimerNode*>  _heap;
    map<int, TimerNode*> _map;
    static int _count;              //节点个数
};

int MinHeapTimer::_count = 0;
