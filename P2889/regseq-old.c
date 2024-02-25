#include <regseq.h>

/*
hidden symbol foo.__regseq_data.beg
symbol object bar
symbol object baz
hidden symbol foo.__regseq_data.end

hidden symbol object foo.__regseq_ptr
inline symbol object foo.__regseq
*/

// One __regseq object is emitted per sequence.
// Duplicate definitions from other shared objects are discarded.
struct __regseq
{
	struct __regseq* next;
	struct __regseq* prev;
};

// One __regseq_ptr object is emitted per subsequence (TU/ELF/program).
// It contains pointers to the beginning and to the end of the data section.
// During runtime initialisation it is added to the linked list of subsequences.
struct __regseq_ptr
{
	struct __regseq node;

	// Pointer to the beginning of the data section.
	void* data_beg;

	// Pointer to the end of the data section.
	void* data_end;
};

// For each __regseq_ptr object P, an initialization call is emitted:
// __regseq_link(&S, &P), where S is the associated __regseq object.
// Calls to __regseq_link on the same __regseq object are externally synchronized.
void __regseq_link(regseq_t const root, regseq_ptr_t const ptr)
{
	regseq_t const node = &ptr->node;
	regseq_t const prev = root->prev;

	// Before linking the node into the list, its own pointers are initialised.
	node->next = root;
	node->prev = prev;

	// Link the new node before the root __regseq object. The new node is now
	// reachable when reverse iterating using regseq_prev. However, it is
	// ignored until also reachable by forward iterating using regseq_next.
	__atomic_store_n(
		&root->prev,
		node,
		__ATOMIC_RELEASE);

	// Link the new node after the last node. The new node is now reachable from
	// both directions and will no longer be ignored during reverse iteration.
	__atomic_store_n(
		&prev->next,
		node,
		__ATOMIC_RELEASE);
}


regseq_ptr_t regseq_begin(regseq_t const root)
{
	return (regseq_ptr_t)root->next;
}

regseq_ptr_t regseq_end(regseq_t const root)
{
	return (regseq_ptr_t)root;
}


static inline regseq_t load_next(regseq_t const node)
{
	return __atomic_load_n(&node->next, __ATOMIC_ACQUIRE);
}

static inline regseq_t load_prev(regseq_t const node)
{
	return __atomic_load_n(&node->prev, __ATOMIC_ACQUIRE);
}

regseq_ptr_t regseq_next(regseq_ptr_t const ptr)
{
	// Load the pointer to next node in the list. Any node reachable in the
	// forward direction is always also reachable in the reverse direction.
	return (regseq_ptr_t)load_next(&ptr->node);
}

regseq_ptr_t regseq_prev(regseq_ptr_t const ptr)
{
	// Let the current node be known as A.
	regseq_t const node = &ptr->node;

	// Load one node B after A in the reverse direction. Nodes reachable in the
	// reverse direction are only considered valid if also reachable from the
	// root in the forward direction.
	regseq_t const prev = load_prev(node);

	// Load one node C after B in the reverse direction.
	regseq_t prev_prev = load_prev(prev);

	// Iterate in the forward direction until either A or B is encountered.
	// If B is encountered, it is known to be reachable in both directions.
	// If A is encountered, the node before it in the forward direction
	// is the next valid node from A in the reverse direction.
	for (;;)
	{
		regseq_t const next = load_next(prev_prev);

		if (next == prev)
		{
			return (regseq_ptr_t)prev;
		}

		if (next == node)
		{
			return (regseq_ptr_t)prev_prev;
		}

		prev_prev = next;
	}
}


void* regseq_data(regseq_ptr_t const ptr)
{
	return ptr->data_beg;
}

size_t regseq_size(regseq_ptr_t const ptr)
{
	return (unsigned char*)ptr->data_end - (unsigned char*)ptr->data_beg;
}
