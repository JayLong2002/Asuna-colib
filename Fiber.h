#include <memory>
#include <functional>
#include <ucontext.h>
#include <iostream>
#include <atomic>

/**
 * 协程只有在TERM状态下才允许重置
 * 使用非对称协程模型，也就是子协程只能和线程主协程切换，而不能和另一个子协程切换
 * 
*/

class Fiber : public std::enable_shared_from_this<Fiber>
{

private:
    // 初始化当前线程的coroutine构造函数，user只能通过get_fiber获得coroutine
    Fiber(); 

public:
    typedef std::shared_ptr<Fiber> ptr;

    /**
     * @brief co_routine state
     * @details Asuna-colib only has three states{Running,Term，Ready}
    */
    enum State{
        READY,
        RUNNING,
        TERM
    };

    // co_routine braek point
    Fiber(std::function<void()> cr_bp);
    
    ~Fiber();

    void reset(std::function<void()> cb);

    void resume();

    void yield();    

    uint64_t get_id() const {return id;};

    State get_state() const {return  state;};


public:
    /**
     * @brief  set t_fiber
     * 
    */
    static void set_fiber(Fiber *f);

    // 使用协程之前必须显式调用一次get_fiber
    static Fiber::ptr get_fiber(); 

    static uint64_t fiber_counts();

    static uint64_t get_fiber_id();

    static void MainFUNC();


private:
    uint64_t id = 0;

    uint32_t stack_size = 128 * 1024;

    State state = READY;

    void *stack = nullptr;
    

    // 入口函数
    std::function<void()> cr_bp;

    bool join_scheduler;

    ucontext_t ctx;
};


/// 全局静态变量，用于生成协程id
static std::atomic<uint64_t> s_fiber_id{0};
/// 全局静态变量，用于统计当前的协程数
static std::atomic<uint64_t> s_fiber_count{0};

/// 线程局部变量，一直指向当前正在运行的协程
static thread_local Fiber *running_fiber = nullptr;

// 当前协程的主协程
static thread_local Fiber::ptr owner_fiber = nullptr;

// stack  size , 默认128k
static const uint32_t fiber_stack_size = 128 * 1024;

//thread_local 在线程开始的时候被生成，在线程结束的时候被销毁，并且每一个线程都拥有一个独立的变量实例