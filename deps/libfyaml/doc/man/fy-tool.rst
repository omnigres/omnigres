fy-tool
=======

Synopsis
--------

**fy-tool** [*OPTIONS*] [<*file*> ...]

**fy-dump** [*OPTIONS*] [<*file*> ...]

**fy-testsuite** [*OPTIONS*] <*file*>

**fy-filter** [*OPTIONS*] [-f*FILE*] [<*path*> ...]

**fy-join** [*OPTIONS*] [-T*PATH*] [-F*PATH*] [-t*PATH*] [<*file*> ...]

**fy-ypath** [*OPTIONS*] <*ypath-expression> [<*file*> ...]

**fy-compose** [*OPTIONS*] [<*file*> ...]

Description
-----------

:program:`fy-tool` is a general YAML/JSON manipulation tool using `libfyaml`.
Its operation is different depending on how it's called, either via aliases
named `fy-dump`, `fy-testsuite`, `fy-filter`, `fy-join`, or via the
`--dump`, `--testsuite`, `--filter`, `--join`, `--ypath` and `--compose` options.

* In `dump` mode it will parse YAML/JSON input files and output YAML/JSON
  according to the output format options.

* In `testsuite` mode it will parse a single YAML input file and output
  yaml testsuite reference output.

* In `filter` mode it will parse YAML/JSON files and output YAML/JSON
  output filtered according to the given option.

* In `join` mode it will parse YAML/JSON files and join them into a single
  YAML/JSON document according to the given command line options.

* In `ypath` mode it will parse YAML/JSON files and execute a ypath query
  which will output a document stream according to the results. This is
  an experimental mode under development, where the syntax is not yet
  decided completely.

* In `compose` mode, it operates similarly to dump, but the document tree
  is created using the built-in composer API.

Options
-------

.. program:: fy-tool

A number of options are common for all the different modes of operation
and they are as follows:

.. rubric:: Common options

.. option:: -q, --quiet

   Quiet operation, does not output informational messages at all.

.. option:: -h, --help

   Display usage information.

.. option:: -v, --version

   Display version information.

.. option:: -I DIR, --include=DIR

   Add the `DIR`  directory to the search path which will be used to locate a YAML/JSON file.
   The default path is set to ""

.. option:: -d LEVEL, --debug-level=LEVEL

   Set the minimum numeric debug level value of the library to `LEVEL`.
   The numeric value must be in the range of 0 to 4 and their meaning is as follows:

   - **0** (`DEBUG`)
      
     Internal library debugging messages. No output is produced when the
     library was compiled with `--disable-debug`
    
   - **1** (`INFO`)

     Informational messages about the internal operation of the library.

   - **2** (`NOTICE`)
     
     Messsages that could require attention.

   - **3** (`WARNING`)

     A warning message, something is wrong, but operation can continue. 
     This is the default value.

   - **4** (`ERROR`)

     A fatal error occured, it is impossible to continue.

   The default level is 3 (`WARNING`), which means that messages
   with level 3 and higher will be displayed.

.. rubric:: Parser Options

.. option:: -j JSONOPT, --json=JSONOPT

   Marks the input files as JSON or YAML accordingly to:

   - **no**

     The input files are always in YAML mode.

   - **force**

     The input files are always set to JSON mode.

   - **auto**

     The input files are set to JSON mode automatically when the
     file's extension is `.json`. This is the default.

   JSON support is complete so all valid JSON files are parsed according
   to JSON rules, even where those differ with YAML rules.

.. option:: --yaml-1.1

   Force YAML 1.1 rules, when input does not specify a version via a directive.

.. option:: --yaml-1.2

   Force YAML 1.2 rules, when input does not specify a version via a directive.

.. option:: --yaml-1.3

   Force YAML 1.3 rules, when input does not specify a version via a directive.
   This option is experimental since the 1.3 spec is not yet released.

.. option:: --disable-accel

   Disable use of acceleration features; use less memory but potentially more CPU.

.. option:: --disable-buffering

   Disable use stdio bufferring, reads will be performed via unix fd reads. This
   may reduce latency when reading from a network file descriptor, or similar.

.. option:: --disable-depth-limit

   Disable the object depth limit, which is usually set to a value near 60.
   Using this option is is possible to process even pathological inputs when
   using the default non-recursive build mode.

.. option:: --prefer-recursive

   Prefer recursive build methods, instead of iterative. This field is merely here
   for evaluation purposes and will be removed in a future version.

.. option:: --sloppy-flow-indentation

   Use sloppy flow indentation, where indentation is not taken into account in flow
   mode, even when the input is invalid YAML according to the spec.

.. option:: --ypath-aliases

   Process aliases using ypaths. Experimental option.

.. option:: --streaming

   Only valid when in **dump** mode, enables streaming mode. This means
   that no in-memory graph tree is constructed, so indefinite and arbitrary
   large YAML input streams can be processed.

   Note that in streaming mode:

   - Key duplication checks are disabled.
   - No reording of key order is possible when emitting (i.e. `--sort` is not available).
   - Alias resolution is not available (i.e. `--resolve`).

.. rubric:: Resolver Options

.. option:: -r, --resolve

   Perform anchor and merge key resolution. By default this option is disabled.

.. option:: -l, --follow

   Follow aliases when performing path traversal. By default this option is disabled.

.. rubric:: Testsuite Options

.. option:: --disable-flow-markers

   Do not output flow-markers for the testsuite output.

.. rubric:: Emitter Options

.. option:: -i INDENT, --indent=INDENT

   Sets the emitter indent (in spaces). Default is **2**.

.. option:: -w WIDTH, --width=WIDTH

   Sets the preferred output width of the emitter. It is generally impossible
   to strictly adhere to this limit so this is treated as a hint at best.
   It not valid in any oneline output modes (i.e. `flow-oneline` or `json-oneline`).
   Default value is 80.

.. option:: -m MODE, --mode=MODE

   Sets the output mode of the YAML emitted. Possible values are:

   - **original**

     The original formatting used in the input. This is default mode.

   - **block** 

     The output is forced to be in block mode. All flow constructs will
     be converted to block mode.

   - **flow**

     The output is forced to be in flow mode. All block constructs will
     be converted to flow mode.

   - **flow-oneline**

     The output is forced to be in flow mode, but no newlines will be
     emitted; the output is going to be a (potentially very) long line.

   - **json**

     The output is forced to be in JSON mode. Note that it is impossible
     to output an arbitrary YAML file as JSON, so this may fail.

   - **json-oneline**

     The output is forced to be in JSON mode and in a single line.

   - **dejson**

     Output is in block YAML mode but with special care to convert
     JSON quoted strings in as non-idiomatic YAML as possible.
     For example `{ foo: "this is a test" }` will be emitted as
     `foo: this is a test`. YAML can handle scalars without using
     excessive quoting.

.. option:: -C MODE, --color=MODE

   It is possible to colorize output using ANSI color escape sequences,
   and the mode can be one of:

   - **off**

     Never colorize output.

   - **on**

     Always colorize output.

   - **auto**

     Automatically colorize output when the output is a terminal.
     This is the default.

.. option:: -V, --visible

   Make all whitespace (spaces, unicode spaces and linebreaks) visible.
   Note that this is performed using UTF8 characters so it will not work
   on non-UTF8 terminals, or a non-UTF8 complete font.

.. option:: -s, --sort

   Sort keys on output. This option is disabled by default.

.. option:: -c, --comment

   Experimental output comments option. Enabled output while preserving comments.
   Disabled by default.

.. option:: --strip-labels

   Strip labels on output. Disabled by default.

.. option:: --strip-tags

   Strip tags on output. Disabled by default.

.. option:: --strip-doc

   Strip document indicators on output. Disabled by default.

.. option:: --null-output

   Do not generate any output, useful for profiling the parser.

.. rubric:: YPATH options

.. option:: --dump-pathexpr

   Dump the produced path expression for debugging.

.. option:: --no-exec

   Do not execute the expression. Useful when used with `--dump-pathexpr`

.. rubric:: Compose options

.. option:: --dump-path

   Dump the path while composing.

.. rubric:: Tool mode select options

.. option:: --dump

   Select `dump` mode of operation. This is the default.
   This mode is also enabled when the called binary is aliased to
   *fy-dump*.

   In this mode, all files provided in the command line will be dumped
   in one continuous stream, to the standard output, using document start
   indicators to mark the start of end new file.

   If the file provided is `-` then the input is the standard input.

.. option:: --testsuite

   Select `testsuite` mode of operation.
   This mode is also enabled when the called binary is aliased to *fy-testsuite*.

   In this mode, a single YAML file is read and an event stream is
   generated which is the format used for *yaml-testsuite* compliance.

   If the file provided is `-` then the input is the standard input.

.. option:: --filter

   Select `filter` mode of operation.
   This mode is also enabled when the called binary is aliased to *fy-filter*.

   In this mode, a single YAML file is read from the standard input for each path
   that is provided in the command line a document will be produced to the
   standard output.
   To use file instead of standard input use the `-f/--file` option.

   If the file provided is `-` then the input is the standard input.

   .. option:: -f FILE, --file=FILE

      Use the given file as input instead of standard input.

      If  first character of `FILE` is **>** the the input is the content of the option
      that follows.
      For example --file ">foo: bar" is as --file file.yaml with file.yaml "foo: bar"

.. option:: --join

   Select `join` mode of operation.
   This mode is also enabled when the called binary is aliased to *fy-join*.

   In this mode, multiple YAML files are joined into a single document, emitted
   to the standard output.

   If the file provided is `-` then the input is the standard input.

   .. option:: -T PATH, --to=PATH

      The target path of the join. By default this is the root **/**.

      If  first character of `FILE` is **>** the the input is the content of the option
      that follows.

   .. option:: -F PATH, --from=PATH

      The origin path of the join (for each input). By default this is the root **/**.

      If  first character of `FILE` is **>** the the input is the content of the option
      that follows.

   .. option:: -t PATH, --trim=PATH

      Trim path of the output of the join. By default this is the root **/**.

      If  first character of `FILE` is **>** the the input is the content of the option
      that follows.

.. option:: --ypath

   Process files and output query results using ypath.

.. option:: --compose

   Use the composer API to build the document instead of direct events.

Examples
--------

.. rubric:: Example input files

We're going to be using a couple of YAML files in our examples.

.. code-block:: yaml
   :caption: invoice.yaml

   # invoice.yaml
   invoice: 34843
   date   : !!str 2001-01-23
   bill-to: &id001
   given  : Chris
   family : Dumars
   address:
           lines: |
           458 Walkman Dr.
           Suite #292

.. code-block:: yaml
   :caption: simple-anchors.yaml

   # simple-anchors.yaml
   foo: &label { bar: frooz }
   baz: *label

.. code-block:: yaml
   :caption: mergekeyspec.yaml

   ---
   - &CENTER { x: 1, y: 2 }
   - &LEFT { x: 0, y: 2 }
   - &BIG { r: 10 }
   - &SMALL { r: 1 }
   
   # All the following maps are equal:
   
   - # Explicit keys
     x: 1
     y: 2
     r: 10
     label: center/big
   
   - # Merge one map
     << : *CENTER
     r: 10
     label: center/big
   
   - # Merge multiple maps
     << : [ *CENTER, *BIG ]
     label: center/big
   
   - # Override
     << : [ *BIG, *LEFT, *SMALL ]
     x: 1
     label: center/big


.. code-block:: yaml
   :caption: bomb.yaml

   a: &a ["lol","lol","lol","lol","lol","lol","lol","lol","lol"]
   b: &b [*a,*a,*a,*a,*a,*a,*a,*a,*a]
   c: &c [*b,*b,*b,*b,*b,*b,*b,*b,*b]
   d: &d [*c,*c,*c,*c,*c,*c,*c,*c,*c]
   e: &e [*d,*d,*d,*d,*d,*d,*d,*d,*d]
   f: &f [*e,*e,*e,*e,*e,*e,*e,*e,*e]
   g: &g [*f,*f,*f,*f,*f,*f,*f,*f,*f]

.. rubric:: fy-dump examples.

Parse and dump generated YAML document tree in the original YAML form

.. code-block:: bash

   $ fy-dump invoice.yaml

.. code-block:: yaml

   invoice: 34843
   date: !!str 2001-01-23
   bill-to: &id001
   given: Chris
   family: Dumars
     address:
     lines: |
       458 Walkman Dr.
       Suite #292

Parse and dump generated YAML document tree in flow YAML form

.. code-block:: bash

   $ fy-dump -mflow invoice.yaml

.. code-block:: yaml

   {
     invoice: 34843,
     date: !!str 2001-01-23,
     bill-to: &id001 {
       given: Chris,
       family: Dumars,
       address: {
         lines: "458 Walkman Dr.\nSuite #292\n"
       }
     }
   }

Parse and dump generated YAML document from the input string

.. code-block:: bash

   $ fy-dump -mjson ">foo: bar"

.. code-block:: json

   {
     "foo": "bar"
   }

Using the resolve option on the `simple-anchors.yaml`

.. code-block:: bash

   $ fy-dump -r simple-anchor.yaml

.. code-block:: yaml

   foo: &label {
       bar: frooz
     }
   baz: {
       bar: frooz
     }

Stripping the labels too:

.. code-block:: bash

   $ fy-dump -r --strip-label simple-anchor.yaml

.. code-block:: yaml

   foo: {
       bar: frooz
     }
   baz: {
       bar: frooz
     }

Merge key support:

.. code-block:: bash

   $ fy-dump -r --strip-label mergekeyspec.yaml

.. code-block:: yaml

   ---
   - {
       x: 1,
       y: 2
     }
   - {
       x: 0,
       y: 2
     }
   - {
       r: 10
     }
   - {
       r: 1
     }
   - x: 1
     y: 2
     r: 10
     label: center/big
   - y: 2
     x: 1
     r: 10
     label: center/big
   - r: 10
     y: 2
     x: 1
     label: center/big
   - y: 2
     r: 10
     x: 1
     label: center/big

Sorting option:

.. code-block:: bash

   $ fy-dump -s invoice.yaml

.. code-block:: yaml

   bill-to: &id001
     address:
       lines: |
         458 Walkman Dr.
         Suite #292
     family: Dumars
     given: Chris
   date: !!str 2001-01-23
   invoice: 34843

.. rubric:: fy-testsuite example.

An example using the testsuite mode generates the following
event stream from `invoice.yaml`

Parse and dump test-suite event format

.. code-block:: bash

   $ fy-testsuite invoice.yaml

.. code-block::

   +STR
   +DOC
   +MAP
   =VAL :invoice
   =VAL :34843
   =VAL :date
   =VAL <tag:yaml.org,2002:str> :2001-01-23
   =VAL :bill-to
   +MAP &id001
   =VAL :given
   =VAL :Chris
   =VAL :family
   =VAL :Dumars
   =VAL :address
   +MAP
   =VAL :lines
   =VAL |458 Walkman Dr.\nSuite #292\n
   -MAP
   -MAP
   -MAP
   -DOC
   -STR

.. rubric:: fy-filter examples.

Filter out from the `/bill-to` path of `invoice.yaml`

.. code-block:: bash

   $ cat invoice.yaml | fy-filter /bill-to

.. code-block:: yaml

   &id001
   given: Chris
   family: Dumars
   address:
     lines: |
       458 Walkman Dr.
       Suite #292

Filter example with arrays (and use the --file option)

.. code-block:: bash

   $ fy-filter --file=mergekeyspec.yaml /5

.. code-block:: yaml

   ---
   <<: *CENTER
   r: 10
   label: center/big

Follow anchors example 

.. code-block:: bash

    $ fy-filter --file=simple-anchors.yaml /baz/bar

.. code-block:: yaml

    frooz

Handle YAML bombs (if you can spare the memory and cpu time)

.. code-block:: bash
   
   $ fy-filter --file=bomb.yaml -r / | wc -l
   6726047

You don\'t have to, you can just follow links to retrieve data.

.. code-block:: bash

    $ fy-filter --file=stuff/bomb.yaml -l --strip-label /g/0/1/2/3/4/5/6

.. code-block:: yaml

    "lol"

Following links works with merge keys too:

.. code-block:: bash

   $ fy-filter --file=mergekeyspec.yaml -l --strip-label /5/x

.. code-block:: yaml

   --- 1

.. rubric:: fy-join examples.

Joining two YAML files that have root mappings.

.. code-block:: bash

   $ fy-join simple-anchors.yaml invoice.yaml

.. code-block:: yaml

   foo: &label {
       bar: frooz
     }
   baz: *label
   invoice: 34843
   date: !!str 2001-01-23
   bill-to: &id001
     given: Chris
     family: Dumars
     address:
       lines: |
         458 Walkman Dr.
         Suite #292

Join two files with sequences at root:

.. code-block:: bash

   $ fy-join -mblock ">[ foo, bar ]" ">[ baz ]"

.. code-block:: yaml

   - foo
   - bar
   - baz

Author
------

Pantelis Antoniou <pantelis.antoniou@konsulko.com>

Bugs
----

* The only supported input and output character encoding is UTF8.
* Sorting does not respect language settings.
* There is no way for the user to specific a different coloring scheme.

See also
--------

:manpage:`libfyaml(1)`
