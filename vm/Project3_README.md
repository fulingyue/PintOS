# Pintos Project 3 VM 说明文档

------

## 一. 设计概览

pintos的第三个项目是在前两个项目的基础上实现虚拟内存，主要干了以下这么几件事：

* 设计并实现了一个 supplemental page table，封装了用户程序使用的虚拟页的所有细节，处理了缺页中断
* 设计并实现了一个 frame table，封装了用户程序使用物理页的所有细节，实现了页面置换的时钟算法
* 设计了一个 swap table，以使用硬盘交换区
* 通过mmap、unmap这两个syscall处理memory mapped files

## 二. 设计细节

### 1. supplemental page table

1）对每个进程维护一个表，记录用户程序可能用到的所有的虚拟页的信息。对每个虚拟页提供以下信息：

- 当前页的状态：FRAME（页框，在物理内存中），SWAP（在交换区上），FILE（在文件系统中）
- key：虚拟页地址
- value：物理页地址 或 交换区上的编号 或 文件信息（origin）
- origin：储存来自文件系统的页的对应细节（文件指针、文件偏移量、页对齐信息等）
- writable：用来指示来自文件系统的页是否允许写回文件系统

2）在发生缺页中断时，先查对应进程的supplemental page table得到出问题的虚拟地址所在的虚拟页的各种信息，再向frame table申请一个物理页（可能会进行页面置换），接着从交换区或文件中load相应的页到刚刚申请到的物理页中，最后重新执行出问题的指令。

3）为了保证线程安全，每次新建/修改/删除页表项的时候都会上锁。

4）每个进程所有的页表项存在一个hash table中，用虚拟页地址作为索引。

### 2. frame table

1）对整个操作系统维护一个表，记录所有用户程序可能用到的所有物理页的信息。对每个物理页提供以下信息：

* 所属进程的指针
* 虚拟页地址
* 物理页地址
* pinned：表示该物理页是否允许被页面置换算法置换出去

2）本项目实现的页面置换算法是全局的时钟算法。

3）Pintos将内存平分成两部分，user pool是给用户程序用的，到project3时只有通过frame table才能替用户程序申请到user pool的物理页，其余所有的动态内存申请（页表、struct thread之类的）得到的都是kernel pool的物理页。

4）同样为了线程安全，在申请物理页、释放物理页等过程时会上锁。

5）所有用户进程的申请到的所有frame存在一个hash table中，用物理页地址作为索引。

### 3. swap table

1）swap table也是全局的，负责和硬盘上的交换区交互，使用了系统提供的block接口。

2）swap table 用block设备块的序号来索引和访问。

3）每次释放一个块时，将它的索引添加到释放列表中，这个块可以在下次申请的时候继续使用。

### 4. 系统调用mmap,unmap

这一部分提供了两个系统调用，处理将一段虚拟地址映射到一个硬盘文件上的相关任务：

* mmap 建立映射
  - 在supplemental page table 处注册这一段虚拟地址所涵盖的所有的虚拟页，并提供对应的文件信息给页表中的origin项
  - 注册的虚拟地址不能“沾染”已经注册过的任何虚拟地址
* unmap 解除映射
  - 将之前注册过的、修改过的页写回硬盘
  - 在supplemental page table 处注销之前注册的页

为了实现上述两个syscall，在struct thread记录了一个map file list，用来维护每个映射文件的状态。

另外还有两个特殊的映射：在load可执行文件的时候，建立code segment上一段虚拟地址到只读的可执行文件的映射，以及data segment上一段地址到可写的静态区的映射（当然这个映射对应的页应该被置换到swap区而不是文件系统中，因为我们不应该修改可执行文件中静态变量的信息）。

## 三. 一些调试问题

* 因为makefile中链接了项目中的所有文件，它们都会被编译，所以有时候缺少了include头文件不会被发现，实际运行时可能会调用一些只有声明没有实体的函数，得到错误的结果。
* 为了得到完整的调试信息，我们中途删去了编译选项中的-O，但是没有得到足够优化一度使project1中的两个测试点始终不得通过。
* 实现虚拟内存前后判断用户访问内存是否合法的方法不一样，应该用#ifdef VM分别处理。
* Makefile.userprog里LDFLAGS要加上max-page-size=0x1000，不然在gcc（非工具链）下无法通过所有测试。
* load可执行文件时并没有读任何可执行文件的内容，只是在supplemental page table处注册了一下而已，运行的时候code segment里出现了page fault后才会真正地去读——所以load完了后不能关闭可执行文件的指针。
