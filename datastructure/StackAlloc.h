#ifndef STACKALLOC_H
#define STACKALLOC_H
#include<memory>
//节点
template <typename T>
struct StackNode
{
    StackNode* pre ;
    T   data ;

    StackNode()
    {
        
    }
    ~StackNode()
    {
    }
};


template<typename T, typename  Allocator = std::allocator<T>>  //指定缺省值
class StackAlloc //链栈
{
public:
    size_t  node_cnt ;
    typedef StackNode<T> Node ;
    //一定要声明template 不然会报错
    typedef typename Allocator::template rebind<Node>::other allocator;

    StackAlloc()
    {
        head_ = nullptr ;
        node_cnt = 0 ;
    }

    ~StackAlloc()
    {
        clear();
    }

    void clear() 
    {
        while(node_cnt >0)
        {
            Node* del = head_ ;
            head_ = head_ ->pre;
            allocator_.destroy(del);
            allocator_.deallocate(del , 1);
            node_cnt -- ;
        }
        return ;
    }


    /*这里 Node() 是一个临时的 Node 对象，它会调用编译器生成的隐式默认构造函数
    ，然后将这个临时对象的内容拷贝到 newNode 指向的内存中。*/
    void push(T data )
    {
        Node* newNode = allocator_.allocate(1) ; 
        if(newNode==nullptr) std::cout<<"内存不足\n";
        allocator_.construct(newNode , Node());
        newNode->pre = head_ ;
        newNode->data = data ;
        head_ = newNode ;
        node_cnt++ ;
    }


    T pop()
    {
        Node* del = head_ ;
        T    del_data = del->data ;
        head_ = head_->pre ;
        allocator_.destroy(del) ;
        allocator_.deallocate(del ,1 );
        node_cnt -- ;
        return del_data ;
    }


    T top()
    {
        return head_->data ;
    }


    size_t count()
    {
        return node_cnt ;
    }
private:
    Node* head_ ;
    allocator allocator_ ;
};


#endif