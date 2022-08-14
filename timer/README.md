定时器类型

1)堆排序          
①涉及到的文件   :minheap.cc\minheap.h        
              

2)跳表          
①涉及到的文件  :skiplist.c\skiplist.h\skl-timer.c                         
②编译指令gcc skiplist.c skl-timer.c -o skl -I./              
                       


3)时间轮          
①涉及到的文件   
②编译指令
gcc timewheel.c tw-timer.c -o tw -I./ -lpthread
③整天流程        
    //1、创建8个线程                        
    //2、初始化定时器              
    //3.增加定时器节点，延迟6s后改变推退出标签              
    //4、创建8个线程管理器管理线程，这个几个线程负责往链表数组添加节点              
    //5、循环遍历数组，执行要过期节点和将高维要过期节点放到低维数组链表里面              
    //6、回收线程等其他资源              
④基础数据结构                  
                  
struct timer_node {                                    
	struct timer_node *next;                                    
	uint32_t expire;			//过期时间                                    
    handler_pt callback;		//回调函数                                    
    uint8_t cancel;				//表示这个节点是否取消执行，是就是1                  
	int id; 					//线程索引，从1开始                  
};                                    
                                    
//链表                                    
typedef struct link_list {                                    
	timer_node_t head;   	//节点                                    
	timer_node_t *tail;   	//后续节点                                    
}link_list_t;                                    
                                    
//定时器管理器
typedef struct timer {                                    
	link_list_t near[TIME_NEAR];                                    
	link_list_t t[4][TIME_LEVEL];                                    
	struct spinlock lock;			//锁                                    
	uint32_t time;					                                    
	uint64_t current;                                    
	uint64_t current_point;  		//记录当前时间                  
}s_timer_t;                                    





















