// clang-format off
#include <postgres.h>
#include <fmgr.h>
// clang-format on

typedef int32 Discriminant;
// This is to basically ensure a theoretical limit of types can be covered by the discriminant
// (although it is generally excessive)
StaticAssertDecl(sizeof(uint32_t) <= sizeof(Oid),
                 "discriminant size should not be able to cover more than there can be types");
StaticAssertDecl(sizeof(uint32_t) == sizeof(Oid), "discriminant size should match that of Oid");

/**
 * Fixed-size type layout
 */
typedef struct {
  Discriminant discriminant;
  char data[FLEXIBLE_ARRAY_MEMBER];
} FixedSizeVariant;

/**
 * Variable-size type layout
 */
typedef struct {
  Discriminant discriminant;
  /**
   * @note If the variant iself is a `varlena`, `data` will embed that `varlena` in its entirety
   * to make it easier to work with (no need to create a new varlena with the reduced size).
   * It increases the size of the overall structure by the size of  the `varlena` header.
   */
  struct varlena data;
} VarSizeVariant;

StaticAssertDecl(offsetof(VarSizeVariant, discriminant) == offsetof(FixedSizeVariant, discriminant),
                 "discriminant offset should be equal");
