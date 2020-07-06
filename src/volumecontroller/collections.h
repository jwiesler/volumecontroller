#ifndef COLLECTIONS_H
#define COLLECTIONS_H
#include <cassert>
#include <iterator>
#include <vector>
#include <algorithm>

template<typename Offset, typename Iterator, typename Comparator>
std::vector<Offset> CreateSortedPermutation(const Iterator begin, const Iterator end, Comparator comp) {
	std::vector<Offset> offsets(std::distance(begin, end));
	std::iota(offsets.begin(), offsets.end(), Offset(0));
	std::sort(offsets.begin(), offsets.end(), [&](const auto &a, const auto &b) {
		return comp(begin[a], begin[b]);
	});
	return offsets;
}

template<typename PermutationIterator, typename Swapper>
void ApplyPermutation(PermutationIterator pBegin, PermutationIterator pEnd, Swapper &&swapper) {
	using Index = typename std::iterator_traits<PermutationIterator>::value_type;

	const Index size = std::distance(pBegin, pEnd);
	for(Index i = 0; i < size; ++i) {
		Index offset = i;

		while(i != pBegin[offset])  {
			const Index next = pBegin[offset];
			swapper(offset, next);
			pBegin[offset] = offset;
			offset = next;
		}
		pBegin[offset] = offset;
	}
}

template<typename Iterator, typename IndexIterator>
void removeIndices(const Iterator begin, const Iterator end, const IndexIterator ibegin, const IndexIterator iend) {
	using Index = typename std::iterator_traits<IndexIterator>::value_type;

	if(ibegin == iend)
		return;

	const Index count = std::distance(begin, end);
	Iterator it = begin + *ibegin;
	for (IndexIterator iit = ibegin; iit < iend; iit++) {
		const Index next = (iit + 1 == iend ? count : *(iit + 1));
		const Index valueCount = next - *iit;
		const Iterator moveBegin = begin + *iit + 1;
		const Iterator moveEnd = begin + next;

		std::move(moveBegin, moveEnd, it);
		it += valueCount;
	}
}

template<typename Iterator, typename PermutationIterator>
void Permutate(Iterator begin, Iterator end, PermutationIterator pbegin, PermutationIterator pend) {
	assert(std::distance(begin, end) == std::distance(pbegin, pend));
	auto it = begin;
	for(PermutationIterator pit = pbegin; pit != pend; ) {
		auto offset = *pit;
		if(offset == std::distance(it, end)) {
			++pit;
			++it;
		} else {
			std::iter_swap(it, begin + offset);
			std::iter_swap(pit, pbegin + offset);
		}
	}
}

#endif // COLLECTIONS_H
