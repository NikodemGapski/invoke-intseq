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

	// Replace a non-std::integer_sequence with itself.
	template <class T>
	struct replace {
		using type = T;
	};

	// Replace empty std::integer_sequence.
	template <class Int, Int... vals>
	struct replace<std::integer_sequence<Int, vals...>> {
		using type = std::integral_constant<Int, 0>;
	};

	// Replace non-empty std::integer_sequence.
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
	struct return_type_raw {
		using type = typename return_type_from_tuple<F, typename replace_all<Args...>::tuple>::type;
	};

	template <class F, class... Args>
	struct return_type {
		using type_raw = typename return_type_raw<F, Args...>::type;
		using type_no_ref = std::remove_reference_t<type_raw>;
		constexpr static bool is_ref = std::is_reference_v<type_raw>;

		using type = typename std::conditional_t<
			is_ref,
			typename std::reference_wrapper<type_no_ref>,
			type_raw
		>;

	};
	template <class F, class... Args>
	using return_type_t = typename return_type<F, Args...>::type;

	// Checking if return value is not void.
	template <class F, class... Args>
	concept not_void = not std::same_as<typename return_type_raw<F, Args...>::type, void>;

	// Counting the number of results to be placed in the array.
	template <class F, class... Args>
	struct result_count {
		constexpr static size_t value = 1;
	};

	template <class F, class First, class... Args>
	struct result_count<F, First, Args...> {
		constexpr static size_t value = result_count<F, Args...>::value;
	};
	
	template <class F, class Int, Int... vals, class... Args>
	struct result_count<F, std::integer_sequence<Int, vals...>, Args...> {
		constexpr static size_t value = sizeof...(vals) * result_count<F, Args...>::value;
	};

	struct EmptyResult {};

	template <size_t N, class T, class... Ts>
	constexpr std::array<T, N> initialize_array(T default_value, Ts... pack) {
		if constexpr (sizeof...(pack) < N) {
			return initialize_array<N>(default_value, pack..., default_value);
		} else {
			return std::array<T, N>{pack...};
		}
	}

	// Recursive caller for no argument.
	template <class F, class... Args>
	struct RecursiveCaller {

		constexpr decltype(auto) operator()(auto& result, size_t& idx, F&& f, Args&&... args) {
			if constexpr (not_void<F, Args...>) {
				if constexpr (return_type<F, Args...>::is_ref) {
					auto ref = std::reference_wrapper(std::forward<decltype(f())>(std::invoke(std::forward<F>(f))));
					if constexpr (std::same_as<decltype(result), EmptyResult&>) {
						// This is a sample call to get a default value.
						return ref;
					} else if (idx > 0) {
						// We skip result[0], as it has already been assigned to
						// with the default value.
						result[idx] = ref;
					}
					++idx;
				} else {
					result[idx++] = std::invoke(std::forward<F>(f)); // TODO: is forward really unnecessary here?
				}
			} else {
				std::invoke(std::forward<F>(f));
			}
		}
	};

	// Recursive caller for non-std::integer_sequence.
	template <class F, class First, class... Args>
	struct RecursiveCaller<F, First, Args...> {

		constexpr decltype(auto) operator()(auto& result, size_t& idx, F&& f, First&& first, Args&&... args) {
			if constexpr (std::is_reference<First>()) {
				auto ref = std::ref(std::forward<First>(first));

				RecursiveCaller<decltype(std::bind_front(std::forward<F>(f), ref)), Args...> r_caller;
				return r_caller(result, idx, std::bind_front(std::forward<F>(f), ref), std::forward<Args>(args)...);
			} else {
				RecursiveCaller<decltype(std::bind_front(std::forward<F>(f), std::forward<First>(first))), Args...> r_caller;
				return r_caller(result, idx, std::bind_front(std::forward<F>(f), std::forward<First>(first)), std::forward<Args>(args)...);
			}
		}
	};

	// Recursive caller for non-empty std::integer_sequence argument.
	template <class F, class Int, Int first_val, Int... vals, class... Args>
	struct RecursiveCaller<F, std::integer_sequence<Int, first_val, vals...>, Args...> {

		constexpr decltype(auto) operator()(auto& result, size_t& idx, F&& f, std::integer_sequence<Int, first_val, vals...>&& first, Args&&... args) {
			// First, call recursively for first_val.
			RecursiveCaller<decltype(std::bind_front(std::forward<F>(f), std::integral_constant<Int, first_val>())), Args...> r_caller;
			if constexpr (not_void<F, std::integer_sequence<Int, first_val, vals...>, Args...> && std::same_as<decltype(result), EmptyResult&>) {
				// We have found a default value.
				return r_caller(result, idx, std::bind_front(std::forward<F>(f), std::integral_constant<Int, first_val>()), std::forward<Args>(args)...);
			} else {
				r_caller(result, idx, std::bind_front(std::forward<F>(f), std::integral_constant<Int, first_val>()), std::forward<Args>(args)...);
			}
			// Then call recursively for vals...
			RecursiveCaller<F, std::integer_sequence<Int, vals...>, Args...> r_caller_seq;
			// ---------- TODO [QUESTION]: Is instantiating a new std::integer_sequence correct? ---------- //
			r_caller_seq(result, idx, std::forward<F>(f), std::integer_sequence<Int, vals...>(), std::forward<Args>(args)...);
		}
	};

	// Recursive caller for empty std::integer_sequence argument.
	template <class F, class Int, class... Args>
	struct RecursiveCaller<F, std::integer_sequence<Int>, Args...> {

		constexpr void operator()(auto& result, size_t& idx, F&& f, std::integer_sequence<Int>&& first, Args&&... args) {
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
			size_t idx = 0;
			RecursiveCaller<F, Args...> r_caller;
			auto result = [&]() constexpr -> auto {
				if constexpr (not_void<F, Args...>) {
					if constexpr (return_type<F, Args...>::is_ref && result_count<F, Args...>::value > 0) {
						// Initialize the array with *some* default value. As we have no guarantee of a default
						// constructor, we have to supply one of the evaluated-to-be values.
						EmptyResult dummy;
						auto default_value = r_caller(dummy, idx, std::forward<F>(f), std::forward<Args>(args)...);
						// Initialize a std::array with default_value.
						return initialize_array<result_count<F, Args...>::value>(default_value);
					} else {
						return std::array<return_type_t<F, Args...>, result_count<F, Args...>::value>();
					}
				} else {
					return EmptyResult();
				}
			}();
			idx = 0;
			r_caller(result, idx, std::forward<F>(f), std::forward<Args>(args)...);

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