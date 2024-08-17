#include <registered_sequence>

inline std::registered_sequence<int> set;

__register(set) constinit int subset_a = 1;
__register(set) constinit int subset_b[] = { 2, 3 };

int main()
{
	for (int const& x : set)
		__builtin_printf("%d\n", x);
}
