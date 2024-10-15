.. Metalang99 documentation master file, created by
   sphinx-quickstart on Mon Jan  4 08:10:23 2021.
   You can adapt this file completely to your liking, but it should at least
   contain the root `toctree` directive.

The Metalang99 Standard Library
===============================

The Metalang99 standard library exports a set of macros implemented using the `Metalang99 metalanguage`_.

Definitions
-----------

 - A plain macro is a macro whose result can be computed only by preprocessor expansion.

 - A Metalang99-compliant macro is a macro called through `ML99_call`/`ML99_callUneval`, directly or indirectly. To compute its result, the Metalang99 interpreter is needed.

 - A desugaring macro is a convenience macro `X(params...)` which expands to `ML99_call(X, params...)` so that you can invoke `X` as `X(v(1), v(2), v(3))`. Desugaring macros are provided for all public Metalang99-compliant macros.

Naming conventions
------------------

 - Plain macros follow the `SCREAMING_CASE` convention.
 - Metalang99-compliant macros follow the `camelCase` convention.
 - Macros denoting language terms (defined by `lang.h`) follow the `camelCase` convention.

Sometimes, there exist two versions of the same macro: one is plain, and the other is Metalang99-compliant. For example, here are two complete metaprograms, one using `ML99_untuple` and the second one using `ML99_UNTUPLE`:

.. code:: c

   ML99_EVAL(ML99_untuple(v((1, 2, 3))))

.. code:: c

   ML99_UNTUPLE((1, 2, 3))

Both metaprograms result in `1, 2, 3`.

Version manipulation macros
---------------------------

*The following macros are defined in metalang99.h*.

`ML99_MAJOR`, `ML99_MINOR`, and `ML99_PATCH` denote the major, the minor, and the patch version numbers, respectively.

`ML99_VERSION_COMPATIBLE(x, y, z)` and `ML99_VERSION_EQ(x, y, z)` are function-like macros that expand to a constant boolean expression:

 - The former holds iff the current Metalang99 version is at least vx.y.z in a `SemVer`_-compatible way. Thus, if the current version is v1.2.3, then `ML99_VERSION_COMPATIBLE` will hold for v1.2.3, v1.2.6, v1.6.0, but not for v2.5.0 or v3.0.0.
 - The latter one holds iff the version is exactly vx.y.z.

These macros can be used as follows:

.. code:: c

    #if !ML99_VERSION_COMPATIBLE(1, 2, 3)
    #error Please, update your Metalang99 to v1.2.3 or higher!
    #endif

.. toctree::
   :hidden:

   lang
   choice
   tuple
   variadics
   list
   seq
   either
   maybe
   nat
   ident
   bool
   util
   assert
   gen
   stmt

Contents
====================================

 - `lang.h`_ - The core metalanguage.
 - `choice.h`_ - Choice types: `(tag, ...)`.
 - `tuple.h`_ - Tuples: `(x, y, z)`.
 - `variadics.h`_ - Variadic arguments: `x, y, z`.
 - `list.h`_ - Cons-lists.
 - `seq.h`_ - Sequences: `(x)(y)(z)`.
 - `either.h`_ - A choice type with two cases.
 - `maybe.h`_ - An optional value.
 - `nat.h`_ - Natural numbers: [0; 255].
 - `ident.h`_ - Identifiers: `[a-zA-Z0-9_]+`.
 - `bool.h`_ - Boolean algebra.
 - `util.h`_ - Utilitary stuff.
 - `assert.h`_ - Static assertions.
 - `gen.h`_ - Support for C language constructions.
 - `stmt.h`_ - Statement chaining.

Indices and tables
====================================

* :ref:`genindex`
* :ref:`search`

.. _Metalang99 metalanguage: https://github.com/Hirrolot/metalang99
.. _SemVer: https://semver.org/

.. _lang.h: lang.html
.. _choice.h: choice.html
.. _tuple.h: tuple.html
.. _variadics.h: variadics.html
.. _list.h: list.html
.. _seq.h: seq.html
.. _either.h: either.html
.. _maybe.h: maybe.html
.. _nat.h: nat.html
.. _ident.h: ident.html
.. _bool.h: bool.html
.. _util.h: util.html
.. _assert.h: assert.html
.. _gen.h: gen.html
.. _stmt.h: stmt.html
