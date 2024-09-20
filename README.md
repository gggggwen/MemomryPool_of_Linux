<font size=4>项目参考https://github.com/cacay/MemoryPool</font> 

如果你像我一样 , 一开始就无脑地研究MemoryPool具体实现, 那么你会对`MemoryPool`的架构逻辑会一头雾水 (本人就是踩了这些坑)。



<font size = 5>**内存池使用的优势:**</font>

- **性能提升**：由于内存池预先分配内存，减少了系统调用的次数，内存分配和释放速度比常规的分配函数要快很多。

- **减少内存碎片**：频繁的内存分配和释放会导致内存碎片问题，使用内存池可以集中管理和分配小块内存，减少碎片化。

- **更好的可控性**：内存池允许程序员精确控制内存的使用和生命周期，从而提高程序的效率和稳定性。

- **批量分配与释放**：可以一次性释放整个内存池中的内存，而不是逐块释放，大大简化了内存管理工作。



[TOC]



# 1. 标准空间配置器

事实上 , 根据**STL** 规范 , **空间配置器具有规定的标准接口** : 无论STL容器支持的分配器`std::allcator<T>` 亦或是 `MemoryPool<T>`实际上都必须遵守该规则。并且在本项目中`StackAlloc.h` 中 StackAlloc容器作为测试样例, 需要比较标准空间配置器`std::allocator` 和 `MemoryPool`性能差异, 需要提供统一的接口

```c++
//这些重定义的变量类型 , 对实现迭代器很重要, 这里就把它当做一种规范即可
allocator::value_type; 	// T
allocator::pointer ;    // *T
allocator::const_pointer ;  //const *T
allocator::reference ;      // &T
allocator::const_reference ;// const &T 
allocator::size_type ;      // size_t
allocator::difference_type ;

//下面会提到 , 很重要!!!将分配器重新绑定
allocate::rebind;


allocator::allocator();
template<class U >allocator::allocator(const allocator<U>&);
allocator::~allocator();

pointer allocator::address(reference x) const ;
const_pointer allocator::address(const_reference x) const ;
 
//配置空间足以存储n个T对象, 第二个参数为提示 可以完全忽略 
void allocator::allocate(size_type n , const void* hint = nullptr);
void allocator::deallocate(pointer p ,size_type n); //n:释放n个T对象
void allocator::construct(pointer p ,const T& X);//构造元素
void allocator::destroy(pointer p); //销毁元素

size_type allocator::max_size() const ;
```



## 	1.1 rebind 机制

具体定义如下:

```c++
template<U>
struct rebind
{
    typedef allocator<U> other ;
}
```

rebind 机制实现了将原本为某种类型设计的分配器重新绑定到另一种类型上 ,分配器在处理不同类型的数据时保持一致的接口。

比方说在`StackAlloc.h`文件中就利用了这样的机制:

```c++
template<typename T>
struct StackNode
{

    T data ;
    StackNode*  pre;
}

template<typename T >
StackAlloc
{
    ....
    typedef StackNode<T> Node ;
    typedef typename Allocator:: template rebind<Node>::other allocator_ ; 
    ....
}
```

由于是在链栈这一容器中进行的存储 , 我们需要将数据封装到各个节点中 ,并且各个节点需要依靠pre指针进行维护 。 **有了这一层封装我们不能直接对类型为T 的变量 直接分配内存** , 而需要rebind机制 进行重新绑定 ,  绑定之后 , 分配器支持对Node 进行空间分配

在我看来这一机制在STL 容器中也发挥了重要的作用 ,容器实际上存放的是封装了数据的元素, 而rebind 机制能够支持对这些元素进行分配空间 。



## 	1.2 allocate , construct , deallocate , destroy

> 标准规定 回去 查书





# 2.内存池

- #### 内存块 block

  一个内存块存放了多组元素 , 由多个内存槽组成  

  为了维护由内存块为节点组成的内存块,  内存块的首个内存槽通常为指针指向前一块内存块首地址 

```c++
/*在allocateblock()函数中*/ 
data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize)); //operator new 后续会讲到
reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_ ;
currentBlock_  = reinterpret_cast<slot_pointer_>(newBlock) ;
```

- #### 内存槽 slot 

  内存块的组成



## 	2.1 成员变量

内存池中一些重要的成员变量

```c++
typedef union Slot_
{
  Slot_*       next ;
  value_type  element ;
}Slot_ ;
typedef Slot_     slot_value_;
typedef Slot_*    slot_pointer_ ;
typedef Slot_&    slot_reference_ ;
typedef char*     data_pointer_;

slot_pointer_     currentBlock_; //指向当前使用内存块首地址
slot_pointer_     freeSlots_ ;   //被释放的内存槽
slot_pointer_     currentSlot_ ; //当前内存槽 (还未被使用)
slot_pointer_     lastSlot_;     //每个内存块的最后一块内存块
```

内存块之间依赖单链表进行维护 ,  被释放的内存槽之间也依靠单链表进行维护 , 那么单链表的维护需要依靠指针, 在数据占用空间很小的情况下 , 依靠指针的空间成本非常之高 , 那么有什么很好的方法能够解决该问题吗?

当然有！

## 	2.2 union Slot_

先看定义:

```c++
typedef union Slot_
{
  Slot_*       next ;
  value_type  element ;
}Slot_ ;
```

Slot_ 为联合体 , 既可以用作维护链表 ,也可以存放数据, 在一定程度上还节省了空间,我认为这是内存池设计的精妙之处之一 ! 

- 当某一块内存槽需要""被释放"时 , 将该内存槽成员变量转成next指针 , 并前插至表头

```c++
/*如deallocate函数*/
template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n ) noexcept
{
    // 移动freeSlots_
    if(freeSlots_)  reinterpret_cast<slot_pointer_>(p)->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
}
```

- 当需要分配空间时 , **优先**分配freeSlots_中的空间

```c++
template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::allocate(size_type n , const_pointer hint ) 
{
    pointer res =nullptr;
    if (freeSlots_ != nullptr)
    { // 分配已释放的内存槽
        res = reinterpret_cast<pointer>(freeSlots_);
        freeSlots_ = freeSlots_->next ;
   }
   else 
    {
        ....
    }
    return res ;
}
```



## 	2.3 padPointer()--内存对齐

定义如下:

```c++
template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer( data_pointer_ p, size_type align) noexcept 
{
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
     return align - (ptr % align);
}
//返回值为:对齐需要填充的字节
//传入参数  p 指针, 以及指针p所存储的数据类型需要对其的字节数
```

在从内存中分配内存块后 , 需要在第一块内存槽向前继续**填充几个字节, 保证内存对齐 ,**

```c++
template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateBlock() noexcept
{   
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));

    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_ ;
    currentBlock_  = reinterpret_cast<slot_pointer_>(newBlock) ; 

    data_pointer_ body = sizeof(slot_pointer_) + newBlock ;
    size_t padding = padPointer(body,alignof(slot_value_)); //需要填充的字节数
    currentSlot_ =  reinterpret_cast<slot_pointer_>(body+padding) ;

    lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_value_)+1);
}
```



## 2.4 operator delete 和operator new

在`~MemoryPool()`中有

```c++
while(currentBlock_!=nullptr)
{
    temp = currentBlock_ ;
    currentBlock_  = currentBlock_->next ;
    operator delete(reinterpret_cast<void*>(temp));
}
```

在`allocatBlock()`中有

```c++
data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));
```



- #### 如何区分 new , delete 和operator new , operator delete 呢?

**<font color= red >前者是对对象的操作 , 而后后者只是纯粹的内存的分配与释放</font>**

new 运行时会调用两个函数 : operator new  和 对象的构造函数

delete运行时会调用两个函数:  对象的析构函数和operator delete 

operator new 和 operator delete 类似于(但不完全是)C 语言中的`malloc` 和`free`他们都是最底层的 , 对于内存的分配和释放的操作, **注意 operator new 运算完的返回值为void* 而 operator delete 只支持对 void*类型指针进行释放**   , 故需要类型强转
