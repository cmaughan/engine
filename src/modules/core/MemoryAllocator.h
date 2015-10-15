#pragma once

#include <memory>

namespace core {

class _DefaultAllocator {
private:
	_DefaultAllocator() {
	}
public:
	static inline void* allocate(size_t count) {
		void* ptr = new unsigned char[count];
		return ptr;
	}

	static inline void deallocate(void* ptr) {
		delete[] ((unsigned char*) ptr);
	}
};

template<class AllocatorClass>
class _MemObject {
public:
	explicit _MemObject() {
	}

	virtual ~_MemObject() {
	}

	inline void* operator new(size_t size) {
		return AllocatorClass::allocate(size);
	}

	inline void* operator new(size_t, void* ptr) {
		return ptr;
	}

	inline void* operator new[](size_t size) {
		return AllocatorClass::allocate(size);
	}

	inline void operator delete(void* ptr) {
		AllocatorClass::deallocate(ptr);
	}

	inline void operator delete(void* ptr, void*) {
		AllocatorClass::deallocate(ptr);
	}

	inline void operator delete[](void* ptr) {
		AllocatorClass::deallocate(ptr);
	}
};

/**
 * @brief define the macro @c ALLOCATOR_CLASS with your own allocator implementation. just create a class
 * with static functions for @c allocate and @c deallocate.
 */
#ifndef ALLOCATOR_CLASS
#define ALLOCATOR_CLASS _DefaultAllocator
#endif
/**
 * @brief Every object that is derived from @c MemObject is allocated with the @c ALLOCATOR_CLASS allocator.
 */
typedef _MemObject<ALLOCATOR_CLASS> MemObject;

}
