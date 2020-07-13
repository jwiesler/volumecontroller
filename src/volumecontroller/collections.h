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
Iterator RemoveIndices(const Iterator begin, const Iterator end, const IndexIterator ibegin, const IndexIterator iend) {
	using Index = typename std::iterator_traits<IndexIterator>::value_type;
	_Adl_verify_range(begin, end);
	_Adl_verify_range(ibegin, iend);

	if(ibegin == iend)
		return end;

	const Index count = std::distance(begin, end);
	Iterator it = begin + *ibegin;
	for (IndexIterator iit = ibegin; iit < iend; iit++) {
		const Index next = (std::next(iit) == iend ? count : *std::next(iit));
		const Iterator moveBegin = begin + *iit + 1;
		const Iterator moveEnd = begin + next;

		it = std::move(moveBegin, moveEnd, it);
	}

	return it;
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

template<typename Tuple, typename Function, size_t ... Indices>
void ForeachTiedInternal(Tuple &&begins, Tuple &&ends, Function && f, std::index_sequence<Indices...>) {
	const auto end = std::get<0>(ends);
	const auto dist = std::distance(std::get<0>(begins), end);
	Q_ASSERT((std::distance(std::get<Indices>(begins), std::get<Indices>(ends)) && ...));
	std::remove_reference_t<std::remove_const_t<Tuple>> iterators = begins;
	for(; std::get<0>(iterators) != end; (++std::get<Indices>(iterators), ...)) {
		std::apply([&](auto &&... iterators) {
			f(*iterators...);
		}, iterators);
	}
}

template<typename Function, typename... Ranges>
void ForeachTied(Function && f, Ranges &&... ranges) {
	static_assert(sizeof...(Ranges) > 0);
	using std::begin;
	using std::end;
	ForeachTiedInternal(std::make_tuple(begin(ranges)...), std::make_tuple(end(ranges)...), std::forward<Function>(f),
							  std::make_index_sequence<sizeof...(Ranges)>{});
}

#endif // COLLECTIONS_H
