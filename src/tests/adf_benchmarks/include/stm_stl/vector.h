#include <stddef.h> // for ptrdiff_t
#include <stdlib.h> // For NULL
#include <new>      // Because you need placement new


// Because you are avoiding std::
// An implementation of swap
template<typename T>
void __attribute__((transaction_safe)) swap(T& lhs,T& rhs)
{
	T   tmp = lhs;
	lhs = rhs;
	rhs = tmp;
}



template <typename Container, typename Value>
class __attribute__((transaction_safe)) vector_iterator
{
	//! vector over which we are iterating.
	Container* my_vector;

	//! Index into the vector
	size_t my_index;

	//! Caches my_vector[my_index]
	/** NULL if cached value is not available */
	mutable Value* my_item;

	template<typename C, typename T>
	friend vector_iterator<C,T> operator+( ptrdiff_t offset, const vector_iterator<C,T>& v );

	template<typename C, typename T, typename U>
	friend bool operator==( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend bool operator!=( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend bool operator<( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend bool operator>( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend bool operator<=( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend bool operator>=( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename T, typename U>
	friend ptrdiff_t operator-( const vector_iterator<C,T>& i, const vector_iterator<C,U>& j );

	template<typename C, typename U>
	friend class vector_iterator;

	template<typename T>
	friend class vector;


	vector_iterator( const Container& vector, size_t index, void *ptr = 0 ) :
		my_vector(const_cast<Container*>(&vector)),
		my_index(index),
		my_item(static_cast<Value*>(ptr))
	{}

public:
	//! Default constructor
	vector_iterator() : my_vector(NULL), my_index(~size_t(0)), my_item(NULL) {}

	vector_iterator( const vector_iterator<Container,Value>& other ) :
		my_vector(other.my_vector),
		my_index(other.my_index),
		my_item(other.my_item)
	{}

	vector_iterator operator+( ptrdiff_t offset ) const {
		return vector_iterator( *my_vector, my_index+offset );
	}
	vector_iterator &operator+=( ptrdiff_t offset ) {
		my_index+=offset;
		my_item = NULL;
		return *this;
	}
	vector_iterator operator-( ptrdiff_t offset ) const {
		return vector_iterator( *my_vector, my_index-offset );
	}
	vector_iterator &operator-=( ptrdiff_t offset ) {
		my_index-=offset;
		my_item = NULL;
		return *this;
	}
	Value& operator*() const {
		Value* item = my_item;
		if( !item ) {
			item = my_item = &((*my_vector)[my_index]);
		}
		return *item;
	}
	Value& operator[]( ptrdiff_t k ) const {
		return (*my_vector)[my_index+k];
	}
	Value* operator->() const {return &operator*();}

	//! Pre increment
	vector_iterator& operator++() {
		size_t k = ++my_index;
		if( my_item ) {
			// Following test uses 2's-complement wizardry
			if( (k& (k-2))==0 ) {
				// k is a power of two that is at least k-2
				my_item= NULL;
			} else {
				++my_item;
			}
		}
		return *this;
	}

	//! Pre-decrement
	vector_iterator& operator--() {
		size_t k = my_index--;
		if( my_item ) {
			// Following test uses 2's-complement trick
			if( (k& (k-2))==0 ) {
				// k is a power of two that is at least k-2
				my_item= NULL;
			} else {
				--my_item;
			}
		}
		return *this;
	}

	//! Post increment
	vector_iterator operator++(int) {
		vector_iterator result = *this;
		operator++();
		return result;
	}

	//! Post decrement
	vector_iterator operator--(int) {
		vector_iterator result = *this;
		operator--();
		return result;
	}

};

template<typename Container, typename T>
vector_iterator<Container,T> operator+( ptrdiff_t offset, const vector_iterator<Container,T>& v ) {
	return vector_iterator<Container,T>( *v.my_vector, v.my_index+offset );
}

template<typename Container, typename T, typename U>
bool operator==( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return i.my_index==j.my_index && i.my_vector == j.my_vector;
}

template<typename Container, typename T, typename U>
bool operator!=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return !(i==j);
}

template<typename Container, typename T, typename U>
bool operator<( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return i.my_index<j.my_index;
}

template<typename Container, typename T, typename U>
bool operator>( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return j<i;
}

template<typename Container, typename T, typename U>
bool operator>=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return !(i<j);
}

template<typename Container, typename T, typename U>
bool operator<=( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return !(j<i);
}

template<typename Container, typename T, typename U>
ptrdiff_t operator-( const vector_iterator<Container,T>& i, const vector_iterator<Container,U>& j ) {
	return ptrdiff_t(i.my_index)-ptrdiff_t(j.my_index);
}


template <typename T>
class __attribute__((transaction_safe)) vector
{
protected:
	typedef size_t size_type;

private:
	unsigned int dataSize;
	unsigned int reserved;
	T*           data;

	template<typename C, typename U>
	friend class vector_iterator;

public:
	typedef T value_type;
	typedef ptrdiff_t difference_type;
	typedef T& reference;
	typedef const T& const_reference;
	typedef T *pointer;
	typedef const T *const_pointer;

	typedef vector_iterator<vector,T> iterator;
	typedef vector_iterator<vector,const T> const_iterator;

	~vector()
		{
			for(unsigned int loop = 0; loop < dataSize; ++loop) {
				// Because we use placement new we must explicitly destroy all members.
				data[loop].~T();
			}
			free(data);
		}

	vector()
		: dataSize(0), reserved(10), data(NULL)
		{
			reserve(reserved);
		}

	vector(const vector<T> &other)
		: dataSize(0), reserved(other.dataSize), data(NULL)
		{
			reserve(reserved);
			dataSize = reserved;
			for(unsigned int loop;loop < dataSize;++loop) {
				// Because we are using malloc/free
						// We need to use placement new to add items to the data
				// This way they are constructed in place
				new (&data[loop]) T(other.data[loop]);
			}
		}

	vector(unsigned int init_num)
		: dataSize(0), reserved(init_num), data(NULL)
		{
			reserve(reserved);
			dataSize = reserved;
			for(unsigned int loop;loop < dataSize;++loop)
				new (&data[loop]) T();
		}

	const vector<T>& operator= (vector<T> x)
		{
			// use copy and swap idiom.
			// Note the pass by value to initiate the copy
			swap(dataSize, x.dataSize);
			swap(reserved, x.reserved);
			swap(data,     x.data);

			return *this;
		}

	void reserve(unsigned int new_size)
		{
			if (new_size < reserved)
				return;

			T*  newData = (T*)malloc(sizeof(T) * new_size);
			if (!newData)
				throw int(2);

			for(unsigned int loop = 0; loop < dataSize; ++loop) {
				// Use placement new to copy the data
				new (&newData[loop]) T(data[loop]);
			}
			swap(data, newData);
			reserved    = new_size;

			for(unsigned int loop = 0; loop < dataSize; ++loop) {
				// Call the destructor on old data before freeing the container.
				// Remember we just did a swap.
				newData[loop].~T();
			}
			free(newData);
		}

	void push_back(const T &item)
		{
			if (dataSize == reserved)
				reserve(reserved * 2);
			// Place the item in the container
			new (&data[dataSize++]) T(item);
		}

	unsigned int  size() const  {return dataSize;}
	bool          empty() const {return dataSize == 0;}

	// Operator[] should NOT check the value of i
	// Add a method called at() that does check i
	const T& operator[] (unsigned i) const      {return data[i];}
	T&       operator[] (unsigned i)            {return data[i];}

	void insert(unsigned int pos, const T& value)
		{
			if (pos >= dataSize)
				throw int(1);

			if (dataSize == reserved)
				reserve(reserved * 2);

			// Move the last item (which needs to be constructed correctly)
			if (dataSize != 0)
				new (&data[dataSize])  T(data[dataSize-1]);

			for(unsigned int loop = dataSize - 1; loop > pos; --loop)
				data[loop]  = data[loop-1];

			++dataSize;

			// All items have been moved up.
			// Put value in its place
			data[pos]   = value;
		}

	void clear()                                        { erase(0, dataSize);}
	void erase(unsigned int erase_index)                { erase(erase_index,erase_index+1);}
	void erase(unsigned int start, unsigned int end)    /* end NOT inclusive so => [start, end) */
	{
		if (end > dataSize)
			end     = dataSize;

		if (start > end)
			start   = end;

		unsigned int dst    = start;
		unsigned int src    = end;
		for(;(src < dataSize);++dst, ++src) { // && (dst < end)
			// Move Elements down;
			data[dst] = data[src];
		}

		unsigned int count = end - start;
		for(;count != 0; --count) {
			// Remove old Elements
			--dataSize;
			// Remember we need to manually call the destructor
			data[dataSize].~T();
		}
	}

	iterator begin()             {return iterator(*this,0);}
	iterator end()               {return iterator(*this,size());}
	const_iterator begin() const {return const_iterator(*this,0);}
	const_iterator end()   const {return const_iterator(*this,size());}

	reference front() {
		return static_cast<T*>(data)[0];
	}
	//! the first item const
	const_reference front() const {
		return static_cast<const T*>(data)[0];
	}
	//! the last item
	reference back() {
		return static_cast<T*>(data)[dataSize > 0 ? dataSize-1 : 0];
	}
	//! the last item const
	const_reference back() const {
		return static_cast<const T*>(data)[dataSize > 0 ? dataSize-1 : 0];
	}


}; //class vector
