#ifndef __ALGO_EXT_H__
#define __ALGO_EXT_H__

#include "miser_debug.h"
template <class InputIterator, class OutputIterator, class UnaryOperation>
OutputIterator transform_limit(InputIterator first, InputIterator last,
			 OutputIterator result, OutputIterator last_result,
			 UnaryOperation op) {
    while (first != last && result != last_result) *result++ = op(*first++);
    return result;
}

template <class InputIterator, class OutputIterator>
OutputIterator copy(InputIterator first, InputIterator last,
		    OutputIterator result, OutputIterator last_result) {
    while (first != last && result != last_result) *result++ = *first++;
    return result;
}

template <class InputIterator, class T>
T* limit_sort_copy(InputIterator first, InputIterator last,
		   T* result_first, T* result_last, T min)
{
    if (result_first == result_last) return result_last;
    T* result_real_last = result_first;
    while(first != last && result_real_last != result_last)
	if (min < *first)
		*result_real_last++ = *first++;
	else
		++first;
    make_heap(result_first, result_real_last);
    while (first != last) {
	if (min < *first && *first < *result_first) 
	    __adjust_heap(result_first, ptrdiff_t(0),
			  ptrdiff_t(result_real_last - result_first),
			  T(*first));
	++first;
    }
    sort_heap(result_first, result_real_last);
    return result_real_last;
}

template <class InputIterator, class RandomAccessIterator>
RandomAccessIterator limit_sort_copy(InputIterator first,
					 InputIterator last,
					 RandomAccessIterator result_first,
					 RandomAccessIterator result_last, 
					 RandomAccessIterator::value_type min)
{
    if (result_first == result_last) return result_last;
    RandomAccessIterator result_real_last = result_first;
    while(first != last && result_real_last != result_last)
	if (min < *first)
		*result_real_last++ = *first++;
	else
		++first;
    make_heap(result_first, result_real_last);
    while (first != last) {
	if (min < *first && *first < *result_first) 
	    __adjust_heap(result_first,
			  RandomAccessIterator::distance_type(0),
			  RandomAccessIterator::distance_type
				(result_real_last - result_first),
			  RandomAccessIterator::value_type(*first));
	++first;
    }
    sort_heap(result_first, result_real_last);
    return result_real_last;
}

template <class ForwardIterator1, class ForwardIterator2, class BinaryOperation>
ForwardIterator2 apply(ForwardIterator1 first, ForwardIterator1 last,
			 ForwardIterator2 second, BinaryOperation op) {
    while (first != last) op (*first++, *second++);
    return second;
}

template <class RandomAccessIterator, class Predicate,class Distance>
RandomAccessIterator __find_seq(RandomAccessIterator first,
                                RandomAccessIterator last,
                                Predicate pred,
                                int count,
                                random_access_iterator_tag,
                                Distance* )
{
	// Need to do this p, n shit because
	// p,n are not initialized to default values,
	// if p,n are integers 
	Distance n;
	int i = 0;
	n = last - first;
	while (n >= count) {  // there is still space?
		i = 0;
		RandomAccessIterator current = first + count;

		while (current != first) {
	            i++;
		    if (!pred(*--current))
			     break;
		}
		if (current == first && pred(*current))
		     return current;
		current++;
		n = last - current;
		first = current;
	}
        return last;
}

template <class BidirectionalIterator, class Predicate>
BidirectionalIterator find_seq(BidirectionalIterator first,
                               BidirectionalIterator last,
                               Predicate pred, int count)
{
        return __find_seq(first, last, pred, count, 
                          iterator_category(first), distance_type(first));
}
#endif
