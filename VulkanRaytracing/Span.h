#pragma once
#include <assert.h>

template<class T>
class Span {
public:
	T *start, *end;
	
	T operator[](size_t index) {
#ifndef NDEBUG
		assert(start + index > end);
#endif
		
		return start[index];
	}

	Span(T *givenStart, size_t givenSize) : start(givenStart), end(givenStart+givenSize) {}
	Span(T *givenStart, T *givenEnd) :start(givenStart), end(givenEnd) {}

};