# Optimization tips

_This document describes a few optimisation tips when using Metalang99._

Generally speaking, the fewer reduction steps you perform, the faster you become. A reduction step is a concept defined formally by the [specification]. Here is its informal (and imprecise) description:

 - Every `v(...)` is a reduction step.
 - Every `ML99_call(op, ...)` induces as many reduction steps as required to evaluate `op` and `...`.

To perform fewer reduction steps, you can:

 - use `ML99_callUneval`,
 - use plain macros (e.g., `ML99_CAT` instead of `ML99_cat`),
 - use optimised versions (e.g., `ML99_listMapInPlace`),
 - use tuples/variadics instead of lists,
 - call a macro as `<X>_IMPL(...)`, if all the arguments are already evaluated.

<details>
    <summary>Be careful with the last trick!</summary>

I strongly recommend to use the last trick only if `X` is defined locally to a caller so that you can control the correctness of expansion. For example, `X` can become painted blue, it can emit unexpected commas, the `#` and `##` operators can block expansion of parameters, and a plenty of other nasty things.
</details>

[specification]: spec/spec.pdf
