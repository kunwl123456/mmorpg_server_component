#include <stdlib.h>
#include <stdio.h>
#include "skiplist.h"

void defaultHandler(zskiplistNode *node) {
}

/* Create a skiplist node with the specified number of levels. */
zskiplistNode *zslCreateNode(int level, unsigned long score, handler_pt func) {
    //给节点配置最大level的空间
    zskiplistNode *zn =
        malloc(sizeof(*zn)+level*sizeof(struct zskiplistLevel));
    zn->score = score;   //头结点过期时间是0
    zn->handler = func;  //头结点默认绑定函数是defaultHandler
    return zn;
}

zskiplist *zslCreate(void) {
    int j;
    zskiplist *zsl = NULL;

    zsl = (zskiplist *)malloc(sizeof(*zsl));
    zsl->level = 1;
    zsl->length = 0;
    zsl->header = zslCreateNode(ZSKIPLIST_MAXLEVEL,0,defaultHandler);
    for (j = 0; j < ZSKIPLIST_MAXLEVEL; j++) {
        zsl->header->level[j].forward = NULL;//头结点的forward都是NULL
    }
    return zsl;
}

/* Free a whole skiplist. */
void zslFree(zskiplist *zsl) {
    zskiplistNode *node = zsl->header->level[0].forward, *next;

    free(zsl->header);
    while(node) {
        next = node->level[0].forward;
        free(node);
        node = next;
    }
    free(zsl);
}

int zslRandomLevel(void) {
    int level = 1;
#if defined(__APPLE__)
    while ((arc4random()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
#else
    //ZSKIPLIST_P * 0xFFFF = 16384
    //rand()&0xFFFF  取低4位16进制数
    //小于1/4的65536就继续
    //原始链表提取到一级索引概率1/4,原始链表提取到二级索引的概率是1/16
    while ((rand()&0xFFFF) < (ZSKIPLIST_P * 0xFFFF))
#endif
        level += 1;
    //这里level表示需要在第一层到第k层添加索引
    //比如说随机到2，只需要在第一层和第二层插入索引
    return (level<ZSKIPLIST_MAXLEVEL) ? level : ZSKIPLIST_MAXLEVEL;
}

/*
score  过期时间
*/
zskiplistNode *zslInsert(zskiplist *zsl, unsigned long score, handler_pt func) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL] = {0}; 
    zskiplistNode *x = NULL;
    int i = 0;
    int level = 0;

    x = zsl->header;//头结点
    for (i = zsl->level-1; i >= 0; i--) {
        while (x->level[i].forward &&
                x->level[i].forward->score < score)
        {
            x = x->level[i].forward;
        }
        update[i] = x;//插第一个节点时，这个数组的0号元素是头结点，其他不做改变
    }

    //随机一个level层数出来
    level = zslRandomLevel();
    printf("zskiplist add node level = %d\n", level);
    
    //高层跳表链表在上面，越底层越接近原始链表
    //如果随机出来的level层数比现在跳表结构的大，那就把数组用头结点填满
    //并更新最大level层
    if (level > zsl->level) {
        for (i = zsl->level; i < level; i++) {
            update[i] = zsl->header;
        }
        zsl->level = level;
    }

    //根据过期时间、回调函数、这个节点的层数 来 创建新节点
    x = zslCreateNode(level,score,func);

    //从第一层到第level层里面都加上这个节点
    for (i = 0; i < level; i++) {
        x->level[i].forward = update[i]->level[i].forward;
        update[i]->level[i].forward = x;
    }

    zsl->length++;
    return x;
}

zskiplistNode* zslMin(zskiplist *zsl) {
    zskiplistNode *x = NULL;
    x = zsl->header;
    return x->level[0].forward;
}

void zslDeleteHead(zskiplist *zsl) {
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL] = {0};
    zskiplistNode *x = zslMin(zsl);
    if (!x) return;
    int i = 0;
    for (i = zsl->level-1; i >= 0; i--) {
        if (zsl->header->level[i].forward == x) {
            zsl->header->level[i].forward = x->level[i].forward;
        }
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}

//删除比较简单，把跨越的层数对应索引删除，再删除节点就行
void zslDeleteNode(zskiplist *zsl, zskiplistNode *x, zskiplistNode **update) {
    int i = 0;
    for (i = 0; i < zsl->level; i++) {
        //若前置节点的后面有要删除的节点，那么就更新forward指针，把该点从单链表删除
        if (update[i]->level[i].forward == x) {
            update[i]->level[i].forward = x->level[i].forward;
        }
    }
    while(zsl->level > 1 && zsl->header->level[zsl->level-1].forward == NULL)
        zsl->level--;
    zsl->length--;
}


void zslDelete(zskiplist *zsl, zskiplistNode* zn) {
    //数组update专门记录前置节点  
    zskiplistNode *update[ZSKIPLIST_MAXLEVEL] = 0;
    zskiplistNode *x = NULL;
    int i = 0;

    x = zsl->header;

    //在各层查找这个节点
    //从最高级的索引开始查找
    for (i = zsl->level-1; i >= 0; i--) {
        //如果后续还存在节点，且节点值小于要删除节点的过期时间，那就更新数组节点信息
        while (x->level[i].forward &&
                x->level[i].forward->score < zn->score)
        {
            x = x->level[i].forward;
        }
        update[i] = x;
    }
    x = x->level[0].forward;

    //如果在原始链表找到该节点(因为前面已经替换到首节点了)
    //传入参数x就是要删除的节点
    if (x && zn->score == x->score) {
        zslDeleteNode(zsl, x, update);
        free(x);
    }
}

//只打印第一层的？
void zslPrint(zskiplist *zsl) {
    zskiplistNode *x = NULL;
    x = zsl->header;
    x = x->level[0].forward;
    printf("start print skiplist level = %d\n", zsl->level);
    int i = 0;
    for (i = 0; i < zsl->length; ++i)
    {
        printf("skiplist ele %d: score = %lu\n", i+1, x->score);
        x = x->level[0].forward;//从前往后遍历
    }
}
