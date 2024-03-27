#include <regseq.h>

#include <assert.h>

#if 1
#define __regseq_printf(...)
#else
#define __regseq_printf __builtin_printf
#endif

// One __regseq_ptr object is emitted per subsequence (TU/ELF/program).
// It contains pointers to the beginning and to the end of the data section.
// During runtime initialisation it is added to the linked list of subsequences.
struct __regseq_ptr
{
	struct __regseq_ptr* next;

	// Pointer to the beginning of the data section.
	void* data_beg;

	// Pointer to the end of the data section.
	void* data_end;
};

// For each __regseq_ptr object P, an initialization call is emitted:
// __regseq_link(&S, &P), where S is the associated __regseq object.
// Calls to __regseq_link on the same __regseq object are externally synchronized.
void __regseq_link(struct __regseq* const root, struct __regseq_ptr* const node)
{
	__regseq_printf("__regseq_link(%p, %p)\n", root, node);

	node->next = NULL;

	struct __regseq_ptr** const p_next = root->tail
		? &root->tail->next
		: &root->head;

	__atomic_store_n(
		p_next,
		node,
		__ATOMIC_RELEASE);

	root->tail = node;

	__regseq_printf("__regseq_link root->head=%p\n", root->head);
	__regseq_printf("__regseq_link root->tail=%p\n", root->tail);
}


regseq_ptr_t regseq_begin(regseq_t const seq)
{
	assert(seq);

	__regseq_printf("regseq_begin(%p)\n", seq);
	__regseq_printf("regseq_begin seq->head=%p\n", seq->head);

	struct __regseq_ptr* const head = __atomic_load_n(
		&seq->head,
		__ATOMIC_ACQUIRE);
	__regseq_printf("regseq_begin head=%p\n", seq->head);

	return head;
}

regseq_ptr_t regseq_end(regseq_t const seq)
{
	return NULL;
}

regseq_ptr_t regseq_next(regseq_ptr_t const ptr)
{
	assert(ptr);
	return __atomic_load_n(
		&ptr->next,
		__ATOMIC_ACQUIRE);
}

void* regseq_data(regseq_ptr_t const ptr)
{
	assert(ptr);
	return ptr->data_beg;
}

size_t regseq_size(regseq_ptr_t const ptr)
{
	assert(ptr);
	return (unsigned char*)ptr->data_end - (unsigned char*)ptr->data_beg;
}
