#include<stdio.h>
#include<cstddef>
#include<climits>
#include<iostream>
#include<utility>
#include<unistd.h>


#ifndef MEMORYPOOL_H
#define MEMORYPOOL_H
template<typename  T , size_t BlockSize = 4096>
class MemoryPool
{
public:
   typedef std::size_t   size_type ;
   typedef T             value_type ;
   typedef T*            pointer ;
   typedef T&            reference ;
   typedef const T      const_value_type ;
   typedef const T*     const_pointer ;
   typedef const T&     const_reference;
   typedef ptrdiff_t    difference_type;

   template<typename  U>
   struct rebind
   {
      typedef MemoryPool<U> other ;
   };

   /*member func*/
   MemoryPool()  noexcept;
   ~MemoryPool() noexcept;
   MemoryPool(const MemoryPool& pool) =delete ;
   MemoryPool(const MemoryPool&& pool ) noexcept ;

   size_type  max_size() ;
   

   MemoryPool& operator=(const MemoryPool& pool) = delete ;
   MemoryPool& operator=(const MemoryPool&& pool) noexcept ;
 
   pointer allocate(size_type n = 1  , const_pointer hint = nullptr)  ;

   inline pointer construct(pointer p,  const_reference val) noexcept; 
   template <typename U> pointer newElement(const_reference val) noexcept;
   template <typename U> inline void destroy(U *p) noexcept; 

   void    deleteElement(pointer p) noexcept ;
   void    deallocate(pointer p, size_type n =1 ) noexcept ;

   private:
   typedef union Slot_
   {
      Slot_*       next ;
      value_type  element ;
   }Slot_ ;
   typedef Slot_     slot_value_;
   typedef Slot_*    slot_pointer_ ;
   typedef Slot_&    slot_reference_ ;
   typedef char*     data_pointer_;

   slot_pointer_     currentBlock_;
   slot_pointer_     freeSlots_ ;
   slot_pointer_     currentSlot_ ;
   slot_pointer_     lastSlot_;

   size_type padPointer(data_pointer_ p, size_type align)  noexcept; //实现内存对齐的函数 

   void allocateBlock() noexcept ;
};

#include "MemoryPool.tcc"
#endif