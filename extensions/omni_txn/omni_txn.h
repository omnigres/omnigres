#ifndef omni_txn_h
#define omni_txn_h

#include <omni/omni_v0.h>

void linearization_init(const omni_handle *handle);
void linearize_transaction();

#endif // omni_txn_h