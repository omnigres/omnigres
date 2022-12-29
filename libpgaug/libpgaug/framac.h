#ifdef __FRAMAC__
/*@
  predicate alloc_size_is_valid(integer sz) = sz <= 0x3fffffff &&
  is_allocable(sz);
  */
// The following specification is adopted from Frama-C's spec for `realloc`
/*@
    allocates \result;
    frees     pointer;
    assigns   __fc_heap_status \from __fc_heap_status;
    assigns   \result \from size, pointer, __fc_heap_status;

    behavior allocation:
      assumes   can_allocate: alloc_size_is_valid(size);
      allocates \result;
      assigns   \result \from size, __fc_heap_status;
      ensures   allocation: \fresh(\result,size);
      ensures   validity: \valid((char*)\result+(0..(size-1)));
      ensures   separation:
         \forall char *p; \true ==>
         \separated ((char *)\result,p);

    behavior fail:
      assumes cannot_allocate: !alloc_size_is_valid(size);
      allocates \nothing;
      frees     \nothing;
      exits \true;

    complete behaviors;
    disjoint behaviors;
  */
extern pg_nodiscard void *repalloc(void *pointer, Size size);
#endif