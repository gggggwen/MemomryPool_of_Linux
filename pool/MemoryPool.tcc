
#ifndef MEMORYPOOL_TCC
#define MEMORYPOOL_TCC

#include"MemoryPool.h" //保证链接接口对接

template <typename T, size_t BlockSize>
typename MemoryPool<T, BlockSize>::size_type
MemoryPool<T, BlockSize>::padPointer( data_pointer_ p, size_type align) noexcept // 传入char* 指针 ,能够实现byte-by-byte 单字节偏移
{
    uintptr_t ptr = reinterpret_cast<uintptr_t>(p);
     return align - (ptr % align);
}

template<typename T ,size_t BlockSize>
typename MemoryPool<T ,BlockSize>::size_type
MemoryPool<T ,BlockSize>::max_size()
{
    size_type maxblocks = -1 / BlockSize ;
    return (BlockSize - sizeof(data_pointer_))/sizeof(slot_value_) * maxblocks ;
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool()  noexcept
{
    currentBlock_ = nullptr; 
    currentSlot_  = nullptr;
    lastSlot_ = nullptr ; 
    freeSlots_ = nullptr ; 
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::~MemoryPool() noexcept
{
    if(currentBlock_ ==nullptr) return ;
    slot_pointer_ temp  ; 
    while(currentBlock_!=nullptr)
    {
        temp = currentBlock_ ;
        currentBlock_  = currentBlock_->next ;
        operator delete(reinterpret_cast<void*>(temp));
    }
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize>::MemoryPool(const MemoryPool &&pool) noexcept
{
    if(this!= &pool)
   {   currentBlock_ = pool.currentBlock_ ; 
        currentSlot_ = pool.currentSlot_ ;
        freeSlots_ = pool.freeSlots_ ;
        lastSlot_ = pool.lastSlot_ ;

        pool.currentSlot_ = nullptr ;
        pool.currentSlot_ = nullptr ;
        pool.lastSlot_    = nullptr ;
        pool.freeSlots_   = nullptr ;
   }
}

template <typename T, size_t BlockSize>
MemoryPool<T, BlockSize> &
MemoryPool<T, BlockSize>::operator=(const MemoryPool &&pool) noexcept
{
    if(this != &pool)
    {
        currentBlock_ = pool.currentBlock_;
        currentSlot_ = pool.currentSlot_;
        freeSlots_ = pool.freeSlots_;
        lastSlot_ = pool.lastSlot_;

        pool.currentSlot_ = nullptr;
        pool.currentSlot_ = nullptr;
        pool.lastSlot_ = nullptr;
        pool.freeSlots_ = nullptr;
    }
    return (*this);
}


template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::allocateBlock() noexcept
{   
    data_pointer_ newBlock = reinterpret_cast<data_pointer_>(operator new(BlockSize));

    reinterpret_cast<slot_pointer_>(newBlock)->next = currentBlock_ ;
    currentBlock_  = reinterpret_cast<slot_pointer_>(newBlock) ; 

    data_pointer_ body = sizeof(slot_pointer_) + newBlock ;
    size_t padding = padPointer(body,alignof(slot_value_));
    currentSlot_ =  reinterpret_cast<slot_pointer_>(body+padding) ;

    lastSlot_ = reinterpret_cast<slot_pointer_>(newBlock + BlockSize - sizeof(slot_value_)+1);
}

template <typename T, size_t BlockSize>
inline typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::construct(pointer p ,const_reference val) noexcept
{
    if( p ==nullptr) return nullptr ;
    new (p) T(val);
    return p ;
}

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
        if(currentSlot_==nullptr||currentSlot_>=lastSlot_) allocateBlock();
        res = reinterpret_cast<pointer>(currentSlot_) ;
        currentSlot_++;  
    }
    return res ;
}


/*销毁内存池*/
template <typename T,size_t BlockSize>
template<typename U>
inline void
MemoryPool<T, BlockSize>::destroy(U* p) noexcept
{
    p->~U(); 
}


//默认的释放空间函数 , 后续或许会进行重载更新
template <typename T, size_t BlockSize>
inline void
MemoryPool<T, BlockSize>::deallocate(pointer p, size_type n ) noexcept
{
    // 移动freeSlots_
    if(freeSlots_)  freeSlots_->next = freeSlots_;
    freeSlots_ = reinterpret_cast<slot_pointer_>(p);
}


template <typename T, size_t BlockSize>
template <typename U>
typename MemoryPool<T, BlockSize>::pointer
MemoryPool<T, BlockSize>::newElement(const_reference val) noexcept
{
    /*分配内存*/
    pointer res = allocate();
    /*构造新对象*/
    construct(res , val );
    return res ;

}

template <typename T, size_t BlockSize>
void MemoryPool<T, BlockSize>::deleteElement(pointer p) noexcept
{
    if(p!=nullptr)
    {
        p->~value_type();
        deallocate( p);
    }
}

#endif