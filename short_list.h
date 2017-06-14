#ifndef SHORT_LIST_H
#define SHORT_LIST_H

// The short_list container is simple STL-like template C++ class
// of doubly-linked list written for my chess program. It is not
// compliant with std::list and can't be used instead.
// Main differences:
// - maximum capacity is 255 elements;
// - ability to store iterator as one element of type 'unsigned char';
// - methods for change position of elements within list;
// - methods for restore erased elements.


// Here is simple example of using the short_list container:

/*

 #include "short_list.h"
 #include <iostream>

 short_list<int, 4> lst;  // create container consists of 4 'int' elements

 for(int i = 0; i < 4; ++i)
    lst.push_front(i);

 for(auto &it : lst)  // for C++11
 {
    it += 1;
    std::cout << it << endl;
 }

*/

// Version: 2.1.0
// Author: Sergey Meus (serg_meus@mail.ru),
// Krasnoyarsk Kray, Russia
// Copyright 2017





//--------------------------------
template <class T, int n = 255>
class short_list
{
///////
public:

    class iterator;
    class reverse_iterator;

    // iterator with size of one unsigned char
    class iterator_entity
    {
        friend class iterator;

        unsigned char num;

    public:
        iterator_entity() {}

        iterator_entity(iterator it)
        {
            num = it.num;
        }

        void operator =(iterator it)
        {
            num = it.num;
        }

        bool operator ==(iterator_entity it) const
        {
            return num == it.num;
        }

        bool operator !=(iterator_entity it) const
        {
            return num != it.num;
        }

        bool operator ==(iterator it) const
        {
            return num == it.num;
        }

        bool operator != (iterator it) const
        {
            return num != it.num;
        }
    };

    class reverse_iterator_entity
    {
        friend class reverse_iterator;

        unsigned char num;

    public:
        reverse_iterator_entity() {}

        reverse_iterator_entity(reverse_iterator it)
        {
            num = it.num;
        }

        void operator =(reverse_iterator it)
        {
            num = it.num;
        }

        bool operator ==(reverse_iterator_entity it) const
        {
            return num == it.num;
        }

        bool operator !=(reverse_iterator_entity it) const
        {
            return num != it.num;
        }

        bool operator ==(reverse_iterator it) const
        {
            return num == it.num;
        }

        bool operator !=(reverse_iterator it) const
        {
            return num != it.num;
        }
    };

    class iterator
    {
        friend class short_list;

    ////////
    private:
        unsigned char num;
        short_list *linked_class;

    ///////
    public:
        iterator() {}

        bool operator ==(iterator it) const
        {
            return num == it.num && linked_class == it.linked_class;
        }

        bool operator !=(iterator it) const
        {
            return num != it.num || linked_class != it.linked_class;
        }

        void operator =(iterator it)
        {
            num = it.num;
            linked_class = it.linked_class;
        }

        void operator =(iterator_entity ite)
        {
            num = ite.num;
        }

        T& operator *() const
        {
            return linked_class->data[num];
        }

        iterator& operator ++()
        {
            num = linked_class->ptr_next[num];
            return *this;
        }

        iterator& operator --()
        {
            num = linked_class->ptr_prev[num];
            return *this;
        }

        iterator operator ++(int)
        {
            iterator tmp = *this;
            ++*this;
            return tmp;
        }

        iterator operator --(int)
        {
            iterator tmp = *this;
            --*this;
            return tmp;
        }

        bool operator ==(iterator_entity ite) const
        {
            return num == ite.num;
        }

        bool operator !=(iterator_entity ite) const
        {
            return num != ite.num;
        }
        unsigned char get_array_index() const
        {
            return this->num;
        }
    };

    class reverse_iterator
    {
        friend class short_list;

    ////////
    private:
        unsigned char num;
        short_list *linked_class;

    ///////
    public:
        reverse_iterator() {}

        bool operator ==(reverse_iterator it) const
        {
            return num == it.num && linked_class == it.linked_class;
        }

        bool operator !=(reverse_iterator it) const
        {
            return num != it.num || linked_class != it.linked_class;
        }

        void operator =(reverse_iterator it)
        {
            num = it.num;
            linked_class = it.linked_class;
        }

        void operator =(reverse_iterator_entity ite)
        {
            num = ite.num;
        }

        T& operator *() const
        {
            return linked_class->data[num];
        }

        reverse_iterator& operator ++()
        {
            num = linked_class->ptr_prev[num];
            return *this;
        }

        reverse_iterator& operator --()
        {
            num = linked_class->ptr_next[num];
            return *this;
        }

        reverse_iterator operator ++(int)
        {
            reverse_iterator tmp = *this;
            ++*this;
            return tmp;
        }

        reverse_iterator operator --(int)
        {
            reverse_iterator tmp = *this;
            --*this;
            return tmp;
        }

        bool operator ==(reverse_iterator_entity ite) const
        {
            return num == ite.num;
        }
        bool operator !=(reverse_iterator_entity ite) const
        {
            return num != ite.num;
        }
        unsigned char get_array_index() const
        {
            return this->num;
        }
    };

    protected:

        enum {MAX_SIZE = 255};

        T data[n + 2];

        unsigned char ptr_prev[n + 1];
        unsigned char ptr_next[n + 1];

        unsigned char _max_size;
        unsigned char _size;

    public:

        short_list()
        {
            this->clear();
        }

        iterator begin();
        iterator end();
        reverse_iterator rbegin();
        reverse_iterator rend();

        bool push_front(T);
        bool push_back(T);


        void erase(iterator);
        void restore(iterator);

        void rerase(reverse_iterator);
        void rrestore(reverse_iterator);

        inline void clear();
        int max_size() const
        {
            return _max_size;
        }

        int size() const
        {
            return _size;
        }
        void direct_swap(iterator it1, iterator it2)
        {
            std::swap(data[it1.num], data[it2.num]);
        }

        void move_element(iterator dst, iterator src);
        void rmove_element(reverse_iterator dst, reverse_iterator src);

        void sort(bool (*less_foo)(T, T));
};





//-----------------------------
template <class T, int n>
void short_list<T, n>::clear()
{
    _max_size   = 0;
    _size       = 0;
    ptr_next[0] = 0;
    ptr_prev[0] = 0;
}





//-----------------------------
template <class T, int n>
bool short_list<T, n>::push_front(T arg)
{
    if(_max_size >= n)
        return false;
    _size++;
    _max_size++;

    unsigned char _first = ptr_next[0];
    ptr_prev[_first]     = _max_size;
    ptr_next[_max_size]  = _first;
    ptr_prev[_max_size]  = 0;
    ptr_next[0]          = _max_size;

    data[_max_size]      = arg;

    return true;
}





//-----------------------------
template <class T, int n>
bool short_list<T, n>::push_back(T arg)
{
    if(_max_size >= n)
        return false;

    _size++;
    _max_size++;

    unsigned char _last = ptr_prev[0];
    ptr_next[_last]     = _max_size;
    ptr_prev[_max_size] = _last;
    ptr_next[_max_size] = 0;
    ptr_prev[0]         = _max_size;

    data[_max_size]     = arg;

    return true;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::erase(iterator it)
{
    if(it.num == 0)
        return;

    unsigned char prv = ptr_prev[it.num];
    unsigned char nxt = ptr_next[it.num];

    ptr_next[prv] = nxt;
    ptr_prev[nxt] = prv;

    _size--;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::rerase(reverse_iterator it)
{
    if(it.num == 0)
        return;
    unsigned char prv = ptr_prev[it.num];
    unsigned char nxt = ptr_next[it.num];

    ptr_next[prv] = nxt;
    ptr_prev[nxt] = prv;

    _size--;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::restore(iterator it)
{
    unsigned char prev = ptr_prev[it.num];
    unsigned char next = ptr_next[it.num];

    ptr_next[prev] = it.num;
    ptr_prev[next] = it.num;

    _size++;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::rrestore(reverse_iterator it)
{
    int prev = ptr_prev[it.num];
    int next = ptr_next[it.num];

    ptr_next[prev] = it.num;
    ptr_prev[next] = it.num;

    _size++;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::move_element(iterator dst, iterator src)                 // place src-th element before dst-th
{
    if(src == dst)
        return;
    unsigned char src_prev = ptr_prev[src.num];
    unsigned char src_next = ptr_next[src.num];

    ptr_next[src_prev] = src_next;
    ptr_prev[src_next] = src_prev;

    unsigned char dst_prev = ptr_prev[dst.num];
    ptr_next[dst_prev] = src.num;
    ptr_prev[src.num] = dst_prev;
    ptr_next[src.num] = dst.num;
    ptr_prev[dst.num] = src.num;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::rmove_element(reverse_iterator dst, reverse_iterator src)
{
    if(src == dst)
        return;
    unsigned char src_rprev = ptr_next[src.num];
    unsigned char src_rnext = ptr_prev[src.num];

    ptr_prev[src_rprev] = src_rnext;
    ptr_next[src_rnext] = src_rprev;

    unsigned char dst_rprev = ptr_next[dst.num];
    ptr_prev[dst_rprev] = src.num;
    ptr_next[src.num] = dst_rprev;
    ptr_prev[src.num] = dst.num;
    ptr_next[dst.num] = src.num;
}





//-----------------------------
template <class T, int n>
void short_list<T, n>::sort(bool (*less_foo)(T data1, T data2))
{
    bool replaced = true;
    if(this->size() <= 1)
        return;

    while(replaced)
    {
        replaced = false;
        iterator it;
        for(it = this->begin(); it != --(this->end()); ++it)
        {
            iterator it_nxt = it;
            ++it_nxt;
            if(less_foo(*it, *it_nxt))
            {
                this->move_element(it, it_nxt);
                replaced = true;
                it = it_nxt;
            }// if(less_foo(
        }// for(it
    }// while(replaced
}


#endif // SHORT_LIST_H

//-----------------------------
template <class T, int n>
typename short_list<T, n>::iterator short_list<T, n>::begin()
{
    iterator it;
    it.num = ptr_next[0];
    it.linked_class = this;
    return it;
}





//-----------------------------
template <class T, int n>
typename short_list<T, n>::reverse_iterator short_list<T, n>::rbegin()
{
    reverse_iterator rit;
    rit.num = ptr_prev[0];
    rit.linked_class = this;
    return rit;
}





//-----------------------------
template <class T, int n>
typename short_list<T, n>::iterator short_list<T, n>::end()
{
    iterator it;
    it.num = 0;
    it.linked_class = this;
    return it;
}





//-----------------------------
template <class T, int n>
typename short_list<T, n>::reverse_iterator short_list<T, n>::rend()
{
    reverse_iterator it;
    it.num = 0;
    it.linked_class = this;
    return it;
}
