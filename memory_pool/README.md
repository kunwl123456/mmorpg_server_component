


memmory_pool analysis:

1）测试内存池对于速度的比较（时间对比,涉及文件test_malloc.cpp和.h）
①一次分配5个对象大小，分配100W次，耗时78767ms，也就是78s（for循环调用500次）
②一次分配500个对象大小，分配1W次，耗时53223ms，也就是53s（for循环调用5次）
③只用malloc分配500W次大小耗时187964ms，也就是187s（for循环调用500W次）
④总结：相差99W次，相差25s，每次分配相差耗时0.025ms,可以看出来用内存池不规定分配数量时其实不耗时，反而只用malloc一个个地分配对象内存就很耗时，大概有2到3倍耗时对比

2）仿造nginx页缓存做内存池


3）模板做内存池







