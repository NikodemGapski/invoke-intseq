#pragma once
#include <concepts>
#include <vector>
#include <ranges>
#include <functional>

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

	template <class Int, Int first, Int... vals>
	struct replace<std::integer_sequence<Int, first, vals...>> {
		using type = std::integral_constant<Int, first>;
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
	template <class F, class... Args>
	using return_type_t = typename return_type<F, Args...>::type;

	// Checking if return value is not void.
	template <class F, class... Args>
	concept not_void = not std::same_as<return_type_t<F, Args...>, void>;

	struct EmptyResult {};

	template <class F, class... Args>
	struct RecursiveCaller {

		constexpr void operator()(auto& result, F&& f, Args&&... args) {
			if constexpr (not_void<F, Args...>) {
				result.push_back(std::invoke(std::forward<F>(f)));
			}
		}
	};

	// Recursive caller for non-std::integer_sequence or no argument.
	template <class F, class First, class... Args>
	struct RecursiveCaller<F, First, Args...> {

		constexpr void operator()(auto& result, F&& f, First&& first, Args&&... args) {
			RecursiveCaller<decltype(std::bind_front(std::forward<F>(f), std::forward<First>(first))), Args...> r_caller;
			r_caller(result, std::bind_front(std::forward<F>(f), std::forward<First>(first)), std::forward<Args>(args)...);
		}
	};

	// Recursive caller for non-empty std::integer_sequence argument.
	template <class F, class Int, Int first_val, Int... vals, class... Args>
	struct RecursiveCaller<F, std::integer_sequence<Int, first_val, vals...>, Args...> {

		constexpr void operator()(auto& result, F&& f, std::integer_sequence<Int, first_val, vals...>&& first, Args&&... args) {
			// First, call recursively for first_val.
			RecursiveCaller<decltype(std::bind_front(std::forward<F>(f), std::integral_constant<Int, first_val>())), Args...> r_caller;
			r_caller(result, std::bind_front(std::forward<F>(f), std::integral_constant<Int, first_val>()), std::forward<Args>(args)...);
			// Then call recursively for vals...
			RecursiveCaller<F, std::integer_sequence<Int, vals...>, Args...> r_caller_seq;
			// ---------- TODO [QUESTION]: Is instantiating a new std::integer_sequence correct? ---------- //
			r_caller_seq(result, std::forward<F>(f), std::integer_sequence<Int, vals...>(), std::forward<Args>(args)...);
		}
	};

	// Recursive caller for empty std::integer_sequence argument.
	template <class F, class Int, class... Args>
	struct RecursiveCaller<F, std::integer_sequence<Int>, Args...> {

		constexpr void operator()(auto& result, F&& f, std::integer_sequence<Int>&& first, Args&&... args) {
			// Do nothing.
		}
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
			// If the function returns something other than void,
			// calculate and store the result.
			constexpr auto result = []() constexpr -> auto {
				if constexpr (not_void<F, Args...>) {
					return std::vector<return_type_t<F, Args...>>();
				} else {
					return EmptyResult{};
				}
			}();
			RecursiveCaller<F, Args...> r_caller;
			r_caller(result, std::forward<F>(f), std::forward<Args>(args)...);

			if constexpr (not_void<F, Args...>) {
				return result;
			}
		}
	};
}

template <class F, class... Args>
constexpr decltype(auto) invoke_intseq(F&& f, Args&&... args) {
	invoke_intseq_h_utils::Caller<F, Args...> caller;
	return caller(std::forward<F>(f), std::forward<Args>(args)...);
}