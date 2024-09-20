#include"./datastructure/DataStructure.h"
#include<chrono>
#include<iostream>
#include<vector>
#define ELEM 50000
struct AA
{
    int a ;
    char b ;
    long long c ;
    long long d ;
    long long e ;
    AA(int aa , char bb , long long cc ,long long dd ): a(aa) , b(bb) ,c(cc) ,d(dd)
    {
        return ;
    }
    AA()
    {
        
    }
};

int main()
{
    StackAlloc<AA, MemoryPool<AA>> Stackpool;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ELEM; i++)
    {
        AA aa(i, 'a', i, i);
        Stackpool.push(aa);
    }

    for (int i = 0; i < ELEM; i++)
    {
        Stackpool.pop();
    }
    auto end = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration = end - start;

    std::cout << "MemoryPool time: " << duration.count() << " ms" << std::endl;

    std::cout<<"------------------------------------\n";

    StackAlloc<AA, std::allocator<AA>> Stackallocator;

    auto start2 = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < ELEM/4; i++)
    {
        AA aa(i, 'b', i, i);
        Stackallocator.push(aa);
        Stackallocator.push(aa);
        Stackallocator.push(aa);
        Stackallocator.push(aa);
    }
    for (int i = 0; i < ELEM/4; i++)
    {
        Stackallocator.pop();
        Stackallocator.pop();
        Stackallocator.pop();
        Stackallocator.pop();
    }
    auto end2 = std::chrono::high_resolution_clock::now();

    std::chrono::duration<double, std::milli> duration2 = end2 - start2;

    std::cout << "std::allocator time: " << duration2.count() << " ms" << std::endl;
}