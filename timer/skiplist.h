#ifndef _MARK_SKIPLIST_
#define _MARK_SKIPLIST_

/* ZSETs use a specialized version of Skiplists */
#define ZSKIPLIST_MAXLEVEL 32 /* Should be enough for 2^64 elements */
#define ZSKIPLIST_P 0.25      /* Skiplist P = 1/2 */

typedef struct zskiplistNode zskiplistNode;
typedef void (*handler_pt) (zskiplistNode *node);

 
struct zskiplistNode {
    // sds ele;
    // double score;
    unsigned long score; // 时间戳，过期时间(跳表节点的数据)
    handler_pt handler;  //处理回调函数
     /*struct zskiplistNode *backward; 从后向前遍历时使用*/

    //用一个数组的结构存放next指针
    struct zskiplistLevel {
        struct zskiplistNode *forward; 
        /* unsigned long span; 这个存储的level间节点的个数，在定时器中并不需要*/ 
    } level[];
};

//比如说原始链表【0】A->B->C->D->E，一级索引是A->C->E->G->,二级索引是A->E->I,三级索引是A->E
//节点A.level[0].forward = 节点B  ，这是原始链表
//节点A.level[1].forward = 节点C  ，这是一级索引
//节点A.level[2].forward = 节点E  ，这是二级索引
//节点A.level[3].forward = 节点I  ，这是三级索引

typedef struct zskiplist {
    // 添加一个free的函数
    struct zskiplistNode *header/*, *tail 并不需要知道最后一个节点*/;
    int length;
    int level;       //表示几层
} zskiplist;

zskiplist *zslCreate(void);               //创建跳表
void zslFree(zskiplist *zsl);             //回收链表资源
zskiplistNode *zslInsert(zskiplist *zsl, unsigned long score, handler_pt func);     //插入跳表节点
zskiplistNode* zslMin(zskiplist *zsl);                                              //
void zslDeleteHead(zskiplist *zsl);                  //删除跳表头结点
void zslDelete(zskiplist *zsl, zskiplistNode* zn);   //删除跳表节点

void zslPrint(zskiplist *zsl);                       //打印跳表节点信息
#endif
