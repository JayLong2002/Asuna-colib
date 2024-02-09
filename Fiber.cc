#include "Fiber.h"
#include <assert.h>


//正在运行的coroutine初始化
void Fiber::set_fiber(Fiber *f){
    running_fiber = f;
}

//default init
Fiber::Fiber(){
    set_fiber(this);
    state = RUNNING;
    
    // 获取当前的上下文到ctx
    if(getcontext(&ctx) < 0){
        std::cout << "Fiber: " << s_fiber_id << "init"<<std::endl;
        assert(false);
    }; 

    ++s_fiber_count;
    id = s_fiber_id++;
}

Fiber::Fiber(std::function<void()> cr_bp)
    :id(s_fiber_id++),cr_bp(cr_bp)
{
    ++s_fiber_count;
    stack_size = fiber_stack_size; 

    // memory on heap can't load elf
    stack = malloc(stack_size);
    
    getcontext(&ctx);
    ctx.uc_link          = nullptr;
    ctx.uc_stack.ss_sp   = stack;
    ctx.uc_stack.ss_size = stack_size;

    //ctx == main_func
    makecontext(&ctx,&Fiber::MainFUNC,0);

}

/**
 * 线程的主协程析构时需要特殊处理，因为主协程没有分配栈和cb
 */
Fiber::~Fiber() {

    --s_fiber_count;
    if (stack) {
        // 有栈，说明是子协程，需要确保子协程一定是结束状态
        assert(state == TERM);
        free(stack);
        std::cout << "dealloc stack, id = " << id << std::endl;
    } else {
        // 没有栈，说明是线程的主协程
        assert(!cr_bp);           // 主协程没有cb
        assert(state == RUNNING); // 主协程一定是执行状态

        auto cur = running_fiber; // 当前协程就是自己
        if (cur == this) {
            set_fiber(nullptr);
        }
    }
}


// 获取当前的协程，每次使用都需要调用
Fiber::ptr Fiber::get_fiber(){
    if(running_fiber){
        return running_fiber->shared_from_this();
    }

    // 如果当前线程还未创建协程，则创建线程的第⼀个协程
    // 并且这个协程为owner_coroutine

    //会在这里初始化fiber(),然后调用set_fiber()
    //完成对running_fiber的初始化
    Fiber::ptr main_fiber(new Fiber);

    assert(running_fiber == main_fiber.get());

    owner_fiber = main_fiber;

    return running_fiber->shared_from_this();
    
}

uint64_t Fiber::fiber_counts(){
    return s_fiber_count;
}

uint64_t Fiber::get_fiber_id(){
    return s_fiber_id;
}

// 协程的执行函数
void Fiber::MainFUNC(){
    auto cur_coroutine = get_fiber();
    assert(cur_coroutine);

    // 断点
    cur_coroutine->cr_bp();

    //协程在结束自动执行yield

    //出来之后reset参数
    cur_coroutine->cr_bp = nullptr;
    cur_coroutine->state = TERM;

    //reset当前cur_coroutine的同时
    //yield到owner_coroutine 
    auto ptr = cur_coroutine.get();
    cur_coroutine.reset();
    ptr->yield(); 
}


//只有状态为term才可以重置
//重置协程就是重复利⽤已结束的协程，复⽤其栈空间，创建新协程
void Fiber::reset(std::function<void()> cb)
{
    assert(state == TERM);
    cr_bp = cb;
    
    getcontext(&ctx);

    ctx.uc_link = nullptr;
    ctx.uc_stack.ss_sp = stack;
    ctx.uc_stack.ss_size = stack_size;

    makecontext(&ctx,&Fiber::MainFUNC,0);
    state = READY;
}

//当前协程和正在运⾏的协程进⾏交换，
//前者状态变为RUNNING，后者状态变为READY
void Fiber::resume(){
    assert(state == READY);
    // change this coroutine in running
    set_fiber(this);
    state == RUNNING;

    if(join_scheduler){
        //scheduler logic 
    }else{
        // 把当前的
        swapcontext(&(owner_fiber->ctx),&ctx);
    }
}   


// call_yield的协程让出执行权
void Fiber::yield(){
    assert(state != READY);
    set_fiber(owner_fiber.get());
    if(state == RUNNING){
        state = READY;
    }
    // change
    swapcontext(&ctx,&(owner_fiber->ctx));
}


