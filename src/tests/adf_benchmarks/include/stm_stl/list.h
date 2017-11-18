
#ifndef L_H_
#define L_H_

//#include <stddef.h> // for ptrdiff_t
#include <stdlib.h> // For NULL
#include <new>      // Because of placement new
#include <utility>


// __addressof is a template and ptrdiff_t is a macro so no need to worry for TM




/*
-------------------------------------------------------------------------------------------------------
    List Node Base
-------------------------------------------------------------------------------------------------------
 */
/// Common part of a node in the list.
struct __attribute__((transaction_safe)) List_node_base
{
	List_node_base* next;
	List_node_base* prev;

	static void swap(List_node_base& x, List_node_base& y) {
		if ( x.next != &x )
		{
			if ( y.next != &y )
			{
				// Both x and y are not empty.
				std::swap(x.next,y.next);
				std::swap(x.prev,y.prev);
				x.next->prev = x.prev->next = &x;
				y.next->prev = y.prev->next = &y;
			}
			else
			{
				// x is not empty, y is empty.
				y.next = x.next;
				y.prev = x.prev;
				y.next->prev = y.prev->next = &y;
				x.next = x.prev = &x;
			}
		}
		else if ( y.next != &y )
		{
			// x is empty, y is not empty.
			x.next = y.next;
			x.prev = y.prev;
			x.next->prev = x.prev->next = &x;
			y.next = y.prev = &y;
		}
	}

	void transfer(List_node_base* const first, List_node_base* const last) {
		if (this != last)
		{
			// Remove [first, last) from its old position.
			last->prev->next  = this;
			first->prev->next = last;
			this->prev->next    = first;

			// Splice [first, last) into its new position.
			List_node_base* const tmp = this->prev;
			this->prev                = last->prev;
			last->prev              = first->prev;
			first->prev             = tmp;
		}
	}

	void reverse() {
		List_node_base* tmp = this;
		do {
			std::swap(tmp->next, tmp->prev);
			tmp = tmp->prev; // Old next node is now prev.
		}
		while (tmp != this);
	}

	void link_before(List_node_base* const position) {
		this->next = position;
		this->prev = position->prev;
		position->prev->next = this;
		position->prev = this;
	}

	void unlink() {
		List_node_base* const next_node = this->next;
		List_node_base* const prev_node = this->prev;
		prev_node->next = next_node;
		next_node->prev = prev_node;
	}
};


/*
-------------------------------------------------------------------------------------------------------
    List Node
-------------------------------------------------------------------------------------------------------
 */

// An actual node in the list.
template<typename Tp>
struct __attribute__((transaction_safe)) List_node : public List_node_base
{
	Tp data;    // User's data.

	template<typename... _Args>
	List_node(_Args&&... args) : List_node_base(), data(std::forward<_Args>(args)...) { }
};

/*
-------------------------------------------------------------------------------------------------------
    List Iterator
-------------------------------------------------------------------------------------------------------
 */

template<typename Tp>
struct __attribute__((transaction_safe)) List_iterator
{
	typedef List_iterator<Tp>                  Self;
	typedef List_node<Tp>                      Node;

//	typedef ptrdiff_t                          difference_type;
//	typedef std::bidirectional_iterator_tag    iterator_category;
	typedef Tp                                 value_type;
	typedef Tp*                                pointer;
	typedef Tp&                                reference;

	List_iterator() : node() { }

	explicit List_iterator(List_node_base* x) : node(x) { }

	// Must downcast from List_node_base to List_node to get to data.
	reference operator*() const {
		return static_cast<Node*>(node)->data;
	}

	pointer operator->() const {
		return std::__addressof(static_cast<Node*>(node)->data);
	}

	Self& operator++() {
		node = node->next;
		return *this;
	}

	Self operator++(int) {
		Self tmp = *this;
		node = node->next;
		return tmp;
	}

	Self& operator--() {
		node = node->prev;
		return *this;
	}

	Self operator--(int) {
		Self tmp = *this;
		node = node->prev;
		return tmp;
	}

	bool operator==(const Self& x) const { return node == x.node; }

	bool operator!=(const Self& x) const { return node != x.node; }

	// The only member points to the list element.
	List_node_base* node;
};

template<typename _Val>
inline bool operator==(const List_iterator<_Val>& x, const List_iterator<_Val>& y)
{ return x.node == y.node; }

template<typename _Val>
inline bool operator!=(const List_iterator<_Val>& x, const List_iterator<_Val>& y)
{ return x.node != y.node; }


/*
-------------------------------------------------------------------------------------------------------
    List Base
-------------------------------------------------------------------------------------------------------
*/

template<typename Tp>
class __attribute__((transaction_safe)) List_base
{
	typedef List_node<Tp>    Node;
protected:

	struct List_impl: public List_node<Tp>
	{
		List_node_base   node;
		size_t           size;

		List_impl() : List_node<Tp>(), node(), size(0) { }
		List_impl(const List_node<Tp>& a) : List_node<Tp>(a), node(), size(0) { }
	};

	List_impl impl;

	// gets the next available node from the pool of nodes
	List_node<Tp>* get_node() {
		List_node<Tp>* tmp = static_cast<List_node<Tp>*>(malloc(sizeof(List_node<Tp>)));
		++impl.size;
		return tmp;
	}

	void put_node(List_node<Tp>* p) {
		free(static_cast<void*>(p));
		--impl.size;
	}

	template<typename... Args>
	void construct(Node* p, Args&&... args) { new ((void *)p) Node(std::forward<Args>(args)...); }

	void destroy(Node* p) { p->~Node(); }

public:

	List_base() : impl() { init(); }
	List_base(const List_node<Tp>& a) : impl(a) { init(); }

	// This is what actually destroys the list.
	~List_base()  { clear(); }

	void clear() {
		typedef List_node<Tp>  Node;
		Node* cur = static_cast<Node*>(impl.node.next);
		while (cur != &impl.node)
		{
			Node* tmp = cur;
			cur = static_cast<Node*>(cur->next);
			destroy(tmp);
			put_node(tmp);
		}
	}

	void init() {
		this->impl.node.next = &this->impl.node;
		this->impl.node.prev = &this->impl.node;
	}
};



/*
-------------------------------------------------------------------------------------------------------
    List
-------------------------------------------------------------------------------------------------------
*/


template<typename Tp>
class __attribute__((transaction_safe)) list : protected List_base<Tp>
{
	typedef List_node<Tp>				             Node;
	typedef List_base<Tp>                            Base;

public:
	typedef Tp                                        value_type;
	typedef Tp*                                       pointer;
	typedef const Tp*                                 const_pointer;
	typedef Tp&                                       reference;
	typedef const Tp&                                 const_reference;
	typedef List_iterator<Tp>                         iterator;
	typedef size_t                                    size_type;
//	typedef ptrdiff_t                                 difference_type;

protected:

	using Base::impl;
	using Base::put_node;
	using Base::get_node;

	// Allocates space for a new node and constructs a copy of args in it.
	template<typename... Args>
	Node* create_node(Args&&... args) {
		Node* p = this->get_node();
		this->construct(p, std::forward<Args>(args)...);
		return p;
	}


public:

	list() : Base() { }
	list(list& other)  : Base() { *this = other; }

	/**
	 *  No explicit dtor needed as the Base dtor takes care of
	 *  things.  The Base dtor only erases the elements, and note
	 *  that if the elements themselves are pointers, the pointed-to
	 *  memory is not touched in any way.  Managing the pointer is
	 *  the user's responsibility.
	 */

	list& operator=(list& x);

	// iterators

	iterator begin() { return iterator(this->impl.node.next); }
	iterator end()   { return iterator(&this->impl.node); }

	bool empty() const  { return this->impl.node.next == &this->impl.node; }
	size_type size() const { return this->impl.size; }

	// element access

	reference front() { return *begin(); }
	reference back() {
		iterator tmp = end();
		--tmp;
		return *tmp;
	}

	// Add data to the front of the %list.
	void push_front(const value_type& x) { this->insert(begin(), x); }

	// Removes first element.
	void pop_front() { this->erase(begin()); }

	// Add data to the end of the %list.
	void push_back(const value_type& x) { this->insert(end(), x); }

	// Removes last element.
	void pop_back() { this->erase(iterator(this->impl.node.prev)); }

	/**
	 *  Constructs object in list before specified iterator.
	 *
	 *  This function will insert an object of type T constructed
	 *  with T(std::forward<Args>(args)...) before the specified
	 *  location.
	 */
	template<typename... Args>
	iterator emplace(iterator position, Args&&... args) {
		Node* tmp = create_node(std::forward<Args>(args)...);
		tmp->link_before(position.node);
		return iterator(tmp);
	}

	// Inserts given value into list before specified iterator.
	iterator insert(iterator position, const value_type& x) {
		Node* tmp = create_node(x);
		tmp->link_before(position.node);
		return iterator(tmp);
	}

    iterator insert(iterator __position, value_type&& x) {
    	return emplace(__position, std::move(x));
    }


	// Remove element at given position.
	iterator erase(iterator position) {
		iterator ret = iterator(position.node->next);
		_M_erase(position);
		return ret;
	}

	// Remove a range of elements.
	iterator erase(iterator first, iterator last) {
		while (first != last)
			first = erase(first);
		return last;
	}

	/**
	 *  Erases all the elements.  Note that this function only erases
	 *  the elements, and that if the elements themselves are
	 *  pointers, the pointed-to memory is not touched in any way.
	 *  Managing the pointer is the user's responsibility.
	 */
	void clear() {
		Base::clear();
		Base::init();
	}

	// Remove all elements equal to value.
	void remove(const Tp& value);

	// Remove consecutive duplicate elements.
	void unique();

	// Merge sorted lists.
	void merge(list& x);

	// Reverse the elements in list.
	void reverse()  { this->impl.node.reverse(); }

//	// Sort the elements.
//	void sort();
//
//	// Sort the elements according to comparison function.
//	template<typename StrictWeakOrdering>
//	void sort(StrictWeakOrdering);


protected:

	// Moves the elements from [first,last) before position.
	void transfer(iterator position, iterator first, iterator last) {
		position.node->transfer(first.node, last.node);
	}

    // Erases element at position given.
    void _M_erase(iterator position) {
      position.node->unlink();
      Node* n = static_cast<Node*>(position.node);
      this->destroy(n);
      put_node(n);
    }


    void _M_insert(iterator position, const value_type& x) {
      Node* tmp = create_node(x);
      tmp->link_before(position.node);
    }

};

// --------------------------------------------------------------------------------------------


template<typename Tp>
list<Tp>& list<Tp>::operator=(list& x)
{
	if (this != &x)
	{
		iterator first1 = begin();
		iterator last1 = end();
		iterator first2 = x.begin();
		iterator last2 = x.end();
		for (; first1 != last1 && first2 != last2;
				++first1, ++first2)
			*first1 = *first2;
		if (first2 == last2)
			erase(first1, last1);
		else {
			for (; first2 != last2; first2++)
				push_back(*first2);
		}
	}
	return *this;
}


template<typename Tp>
void list<Tp>::remove(const value_type& value)
{
	iterator first = begin();
	iterator last = end();
	iterator extra = last;
	while (first != last)
	{
		iterator next = first;
		++next;
		if (*first == value)
		{
			if (std::__addressof(*first) != std::__addressof(value))
				erase(first);
			else
				extra = first;
		}
		first = next;
	}
	if (extra != last)
		erase(extra);
}


template<typename Tp>
void list<Tp>::unique()
{
	iterator first = begin();
	iterator last = end();
	if (first == last)
		return;
	iterator next = first;
	while (++next != last)
	{
		if (*first == *next)
			erase(next);
		else
			first = next;
		next = first;
	}
}


template<typename Tp>
void list<Tp>::merge(list& x)
{
	if (this != &x)
	{
		check_equal_allocators(x);

		iterator first1 = begin();
		iterator last1 = end();
		iterator first2 = x.begin();
		iterator last2 = x.end();
		while (first1 != last1 && first2 != last2)
			if (*first2 < *first1)
			{
				iterator next = first2;
				transfer(first1, first2, ++next);
				first2 = next;
			}
			else
				++first1;
		if (first2 != last2)
			transfer(last1, first2, last2);

		this->impl.size += x.size();
		x.impl.size = 0;
	}
}



//template<typename Tp, typename Alloc>
//void list<Tp, Alloc>::sort()
//{
//	// Do nothing if the list has length 0 or 1.
//	if (this->impl.node.next != &this->impl.node
//			&& this->impl.node.next->next != &this->impl.node)
//	{
//		list carry;
//		list tmp[64];
//		list * fill = &tmp[0];
//		list * counter;
//
//		do
//		{
//			carry.splice(carry.begin(), *this, begin());
//
//			for(counter = &tmp[0];
//					counter != fill && !counter->empty();
//					++counter)
//			{
//				counter->merge(carry);
//				carry.swap(*counter);
//			}
//			carry.swap(*counter);
//			if (counter == fill)
//				++fill;
//		}
//		while ( !empty() );
//
//		for (counter = &tmp[1]; counter != fill; ++counter)
//			counter->merge(*(counter - 1));
//		swap( *(fill - 1) );
//	}
//}




#endif /* L_H_ */
