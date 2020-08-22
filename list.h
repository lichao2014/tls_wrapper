#pragma once

#include <type_traits>
#include <memory>

template<typename NODE, NODE* (NODE::* NEXT)>
class List
{
public:
    List(const List&) = delete;
    void operator=(const List&) = delete;

    List() = default;
    ~List() { clear(); }

    template<typename F>
    void visit(F f)
    {
        NODE* p = _head;
        while (p)
        {
            f(*p);
            p = (p->*NEXT);
        }
    }

    template<typename ... Args>
    void push_back(Args&& ... args) noexcept(std::is_nothrow_constructible_v<NODE, Args...>)
    {
        push_back_node(std::make_unique<NODE>(std::forward<Args>(args)...));
    }

    void push_back_node(std::unique_ptr<NODE> node) noexcept
    {
        *_tail = node.release();
        _tail = &((*_tail)->*NEXT);
    }

    void clear()
    {
        while (true)
        {
            NODE* p = _head;
            if (!p)
            {
                break;
            }

            _head = (p->*NEXT);
            delete p;
        }

        _tail = &_head;
    }
private:
    NODE* _head = nullptr;
    NODE** _tail = &_head;
};
