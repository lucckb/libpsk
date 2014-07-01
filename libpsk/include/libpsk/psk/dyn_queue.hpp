/*----------------------------------------------------------*/
/*
 * fixed_queue.hpp
 *
 *  Created on: 2009-12-09
 *      Author: lucck
 */
/*----------------------------------------------------------*/
#ifndef FND_DYN_QUEUE_HPP_
#define FND_DYN_QUEUE_HPP_
/*----------------------------------------------------------*/
#include <cstddef>

/*----------------------------------------------------------*/
namespace fnd
{
/*----------------------------------------------------------*/
template <typename T>
class dyn_queue {
public:
	//Fixed queue constructor
	explicit dyn_queue( size_t len )
		: buf(new T[len]), head(0),tail(0), Sz(len)
	{
	}
	//Destructor
	~dyn_queue() {
		delete [] buf;
	}
	//Push element
	bool push_back(const T &c) {
		std::size_t next;
		// check if FIFO has room
		next = (head + 1) % Sz;
		if (next == tail) return true;
		buf[head] = c;
		head = next;
		return false;
	}
    //Pop back
	bool pop_back( T &c ) {
		if (head == tail) return true;
		std::size_t prev;
		prev = (head-1) % Sz;
		c = buf[prev];
		head = prev;
		return false;
	}
    //Pop back2
	bool pop_back( ) {
		if (head == tail) return true;
		std::size_t prev;
		prev = (head-1) % Sz;
		head = prev;
		return false;
	}
	//! Get front element
	T& front() { 
		return buf[tail]; 
	}
	const T& front() const { 
		return buf[tail];
	}
	//! Get the back element
	T& back() {
		std::size_t prev;
		prev = (head -1) % Sz;
		return buf[prev];
	}
	const T& back() const {
		std::size_t prev;
		prev = (head -1) % Sz;
		return buf[prev];
	}
	//Pop element
	bool pop_front(T &c) {
		std::size_t next;
		// check if FIFO has data
		if (head == tail) return true;
		next = (tail + 1) % Sz;
		c = buf[tail];
		tail = next;
		return false;
	}
	//Pop element
	bool pop_front() {
		std::size_t next;
		// check if FIFO has data
		if (head == tail) return true;
		next = (tail + 1) % Sz;
		tail = next;
		return false;
	}
	//Get size
	std::size_t size() const {
		return (Sz + head - tail) % Sz;
	}
	bool empty() const {
		return size()==0;
	}
	//Get free count
	std::size_t free() const {
		return (Sz - 1 - size());
	}
	//Clear the fifo context
	void erase() {
		head = 0;
		tail = 0;
	}
private:
	T* const buf;
	std::size_t head;
	std::size_t tail;
	std::size_t Sz;
};

/*----------------------------------------------------------*/

}
/*----------------------------------------------------------*/
#endif /* FIXED_QUEUE_HPP_ */
