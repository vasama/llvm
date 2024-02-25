#ifndef __regseq_h
#define __regseq_h

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

struct __regseq
{
	struct __regseq_ptr* head;
	struct __regseq_ptr* tail;
};

typedef struct __regseq const* regseq_t;
typedef struct __regseq_ptr const* regseq_ptr_t;

regseq_ptr_t regseq_begin(regseq_t seq);
regseq_ptr_t regseq_end(regseq_t seq);
regseq_ptr_t regseq_next(regseq_ptr_t ptr);

void* regseq_data(regseq_ptr_t ptr);
size_t regseq_size(regseq_ptr_t ptr);

#ifdef __cplusplus
} // extern "C"
#endif

#endif
