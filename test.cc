#include "Fiber.h"
#include <string>
#include <vector>
 
void run_in_fiber2() {
    std::cout << "run_in_fiber2 begin" << std::endl;;
    std::cout << "run_in_fiber2 end" << std::endl;;
}
 
void run_in_fiber() {
    std::cout << "fiber begin" << std::endl;;
    /**
     * 非对称协程，子协程不能创建并运行新的子协程，下面的操作是有问题的，
     * 子协程再创建子协程，原来的主协程就跑飞了
     */
    Fiber::ptr fiber(new Fiber(run_in_fiber2));
    fiber->resume();
    std::cout << "fiber end" << std::endl;;

}
 
int main(int argc, char *argv[]) {

    std::cout << "main begin" << std::endl;;

    Fiber::get_fiber();

    auto fiber(new Fiber(run_in_fiber));
    
    fiber->resume();// 转移到run_in_fiber
 
    std::cout << "main end" << std::endl;;
    return 0;
} 