#include <regseq.h>

extern regseq_t set;

__register(set) int const subset_a = 1;
__register(set) int const subset_b[] = { 2, 3 };

int main()
{
	for (regseq_ptr_t p = regseq_begin(set); p; p = regseq_next(p))
	{
		int const* const q = (int const*)regseq_data(p);
		size_t const s = regseq_size(p) / sizeof(int);

		for (size_t i = 0; i < s; ++i)
			__builtin_printf("%d\n", q[i]);
	}
}
