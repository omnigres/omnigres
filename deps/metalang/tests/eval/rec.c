#include <metalang99/eval/rec.h>

#include <metalang99/assert.h>

#include <metalang99/priv/util.h>

#include <metalang99/nat/eq.h>
#include <metalang99/nat/inc.h>

#define F(acc, i)          ML99_PRIV_IF(ML99_PRIV_NAT_EQ(i, 10), F_DONE, F_PROGRESS)(acc, i)
#define F_DONE(acc, _i)    ML99_PRIV_REC_CONTINUE(ML99_PRIV_REC_STOP)(~, acc)
#define F_PROGRESS(acc, i) ML99_PRIV_REC_CONTINUE(F)(acc##X, ML99_PRIV_INC(i))
#define F_HOOK()           F

#define XXXXXXXXXX 678

ML99_ASSERT_UNEVAL(ML99_PRIV_REC_UNROLL(F(, 0)) == 678);

#undef F
#undef F_DONE
#undef F_PROGRESS
#undef F_HOOK
#undef XXXXXXXXXX

int main(void) {}
