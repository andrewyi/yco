# yi's coroutine

### 使用方法
* 参看 `main.c`
* 首先 `yco_init()` 初始化
    * 结束记得 `yco_cleanup()`，否则routine分配的内存和stack内存不会被释放
* `yco_attr_init` 初始化创建协程需要的属性，主要为
    * `is_joinable` 是否需要显式地join coroutine才能回收coroutine所用资源
    * `stack_size` 栈大小，默认为 256KB

* `yco_create` 创建协程
    * `yco_co_t` 表示协程标识符（其实就是协程struct的地址），可为NULL
        * 如果协程创建失败，将被赋值为 0xFFFFFFFFFFFFFFFF
    * `f` 函数，不可为NULL
    * `input` 类型为 void *的输入参数，可为NULL
    * `output` f执行结果将被写入到此地址，可为NULL
    * `attr` 可为NULL

* `yco_join` 等待此 `yco_co_t` 的协程结束
    * 当前协程按照创建顺序逐一执行，没有细化的调度方式

* `yco_schedule` 让出cpu，此时中断当前协程被选择下一个协程继续执行

### 实现细节

* 参看 `yco.c` 和 `yco_ctx.s`
