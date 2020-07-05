#ifndef COMPTR_H
#define COMPTR_H

#include <memory>

#define GET_INTO(into, type, name, pName, expr) {\
	type * pName;\
	expr;\
	name = into<type>(pName);\
}

#define GET_INTO_COMPTR(type, name, pName, expr) GET_INTO(ComPtr, type, name, pName, expr)

#define GET_INTO_COMMEMORYPTR(type, name, pName, expr) GET_INTO(ComMemoryPtr, type, name, pName, expr)

template<typename T>
struct ComPtrRelease {
	void operator()(T *ptr) const {
		ptr->Release();
	}
};

template<typename T>
using ComPtr = std::unique_ptr<T, ComPtrRelease<T>>;

#endif // COMPTR_H
