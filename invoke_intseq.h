#pragma once
#include <concepts>
#include <ranges>
#include <vector>

namespace invoke_intseq_h_utils {
	// Checking if type is a certain template.
	template <class T, template <class Int, Int...> class Template>
	struct is_template : std::false_type {};

	template <template <class Int, Int...> class Template, class Int, Int... args>
	struct is_template<Template<Int, args...>, Template> : std::true_type {};

	// Checking if type is a std::integer_sequence.
	template <class T>
	concept is_intseq = is_template<T, std::integer_sequence>::value;

	// Checking if a type pack contains a std::integer_sequence.
	template <class... Args>
	concept contains_intseq = (is_intseq<Args> || ...);

	// Replacing std::integer_sequence with its leading type.
	template <class T>
	struct replace {
		using type = T;
	};

	template <class Int, Int... vals>
	struct replace<std::integer_sequence<Int, vals...>> {
		using type = Int;
	};

	// Replacing all occurences of std::integer_sequence.
	template <class... T>
	struct replace_all {
		using tuple = std::tuple<typename replace<T>::type...>;
	};

	// Extracting return type from function and argument tuple.
	template <class F, class tuple_t>
	struct return_type_from_tuple {};

	template <class F, class... Args>
	struct return_type_from_tuple <F, std::tuple<Args...>> {
		using type = std::invoke_result_t<F, Args...>;
	};

	// Extracting return type from function and argument pack.
	template <class F, class... Args>
	struct return_type {
		using type = typename return_type_from_tuple<F, typename replace_all<Args...>::tuple>::type;
	};

	// Caller with no std::integer_sequence in Args...
	template <class F, class... Args>
	struct Caller {
		constexpr decltype(auto) operator()(F&& f, Args&&... args) {
			return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
		}
	};

	// Caller with at least one std::integer_sequence in Args...
	template <class F, class... Args>
	requires contains_intseq<Args...>
	struct Caller<F, Args...> {
		constexpr decltype(auto) operator()(F&& f, Args&&... args) {
			std::vector<return_type<F, Args...>> result;

			return result;
		}

		// TODO:
		// - recursive calls to different Callers?
		// - handling std::integer_sequence
		// - handling non-std::integer_sequence
		constexpr decltype(auto) recursive(F&& f, )
	};
}

template <class F, class... Args>
constexpr decltype(auto) invoke_intseq(F&& f, Args&&... args) {
	invoke_intseq_h_utils::Caller<F, Args...> caller;
	return caller(std::forward<F>(f), std::forward<Args>(args)...);
}