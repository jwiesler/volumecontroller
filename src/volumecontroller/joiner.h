#ifndef JOINER_H
#define JOINER_H

#include <utility>

template<typename Stream, typename Iterator, typename Separator, typename Printer>
void JoiningPrint(Stream &stream, const Iterator begin, const Iterator end, Separator separator, Printer &&printer) {
	for(auto it = begin; it != end; ++it) {
		if(it != begin)
			stream << separator;
		printer(stream, *it);
	}
}

template<typename Collection, typename Separator, typename Printer>
class Joiner {
	const Collection &collection;
	Separator separator;
	Printer &&printer;

public:
	Joiner(const Collection &collection, Separator separator, Printer &&printer)
		: collection(collection), separator(separator), printer(std::forward<Printer>(printer)) {}

	template<typename Stream>
	friend Stream &operator<<(Stream &stream, Joiner &&joiner) {
		using std::begin;
		using std::end;
		JoiningPrint(stream, begin(joiner.collection), end(joiner.collection), joiner.separator, std::forward<Printer>(joiner.printer));
		return stream;
	}
};

template<typename Collection, typename Separator, typename Printer>
Joiner(const Collection &, Separator, Printer &&) -> Joiner<Collection, Separator, Printer>;

template<typename Collection, typename Separator, typename Printer>
auto Join(const Collection &collection, Separator separator, Printer &&printer) {
	return Joiner(collection, separator, std::forward<Printer>(printer));
}

template<typename Collection, typename Separator>
auto Join(const Collection &collection, Separator separator) {
	return Join(collection, separator, [](auto &str, const auto &value) {
		str << value;
	});
}

#endif // JOINER_H
