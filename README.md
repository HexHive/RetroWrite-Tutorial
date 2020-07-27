# RetroWrite: fuzzing native binaries at lightning speed

Modern grey-box fuzzers are the most effective way of finding bugs in complex
code bases, and instrumentation is fundamental to their effectiveness. Existing
instrumentation techniques either require source code (e.g., afl-gcc, ASan) or
have a high runtime performance cost (roughly 10x slowdown for e.g., afl-qemu).

We introduce Retrowrite, a binary rewriting framework that enables direct static
instrumentation for both user-mode binaries and Linux kernel modules. Unlike
dynamic translation and trampolining, rewriting code with Retrowrite does not
introduce a performance penalty. We show the effectiveness of Retrowrite for
fuzzing by implementing binary-only coverage tracking and ASan instrumentation
passes. Our binary instrumentation achieves performance similar to
compiler-based instrumentation.

This tutorial consists of three practical phases:

* Fuzzing native binaries without instrumentation
* Instrumenting native binaries with coverage tracking
* Instrumenting native binaries with coverage tracking and address sanitization


## Research background

Fuzzing is the method of choice for finding security vulnerabilities in software
due to its simplicity and scalability, but it struggles to find deep paths in
complex programs, and only detects bugs when the target crashes. Instrumentation
greatly helps with both issues by (i) collecting coverage feedback, which drives
fuzzing deeper into the target, and (ii) crashing the target immediately when
bugs are detected, which lets the fuzzer detect more bugs and produce more
precise reports. One of the main difficulties of fuzzing closed-source software
is that instrumenting compiled binaries comes at a huge performance cost. For
example, simple coverage instrumentation through dynamic binary translation
already incurs between 10x and 100x slowdown, which prevents the fuzzer from
finding interesting inputs and bugs.

We show how we used static binary rewriting for instrumentation: our approach
has low overhead (comparable to compile-time instrumentation) but works on
binaries. There are three main techniques to rewrite binaries: recompilation,
trampoline insertion and reassembleable assembly. Recompilation is the most
powerful but it requires expensive analysis and type recovery, which is an
open/unsolved problem. Trampolines add a level of indirection and increase the
size of the code, both of which have a negative impact on performance.
Reassembleable assembly, the technique that we use, suffers from neither
problem. In order to produce reassembleable assembly, we first disassemble the
binary and then symbolize all code and data references (replacing offsets and
references with references to unique labels). The output can then be fed to a
standard assembler to produce a binary. Because symbolization replaces all
references with labels, we can now insert instrumentation in the code and the
assembler will fix the references for us when reassembling the binary.
Symbolization is possible because references to code and global data always use
RIP-relative addressing in the class of binaries that we support
(position-independent x86\_64 binaries). This makes it easy to distinguish
between references and integer constants in the disassembly.

See the following resources for more details:

* [RetroWrite homepage](https://github.com/HexHive/RetroWrite)
* [RetroWrite paper](http://nebelwelt.net/files/20Oakland.pdf)


## Tutorial outline

We present Retrowrite, a framework for static binary rewriting that can add
efficient instrumentation to compiled binaries and scales to real-world code.
Retrowrite symbolizes x86_64 position-independent Linux binaries and emits
reassembleable assembly, which can be fed to instrumentation passes. We
implement a binary version of Address Sanitizer (ASan) that integrates with the
source-based version. Retrowrite’s output can also be fed directly to AFL’s
afl-gcc to produce a binary with coverage-tracking instrumentation. RetroWrite
is openly available for everyone to use and we will demo it during the
presentation.

Based on the open-source release of RetroWrite, we will rewrite several binaries
(starting with small examples to larger real programs and libraries) and prepare
them for fuzzing and sanitization. Using the demo programs the attendees will
discover bugs in these binaries and learn how to patch them alongside.


The tutorial consists of three phases:

* [Phase 01: fuzzing native binaries without instrumentation](./01-native_fuzzing/README.md)
* [Phase 02: instrumenting native binaries with coverage tracking](./02-coverage_fuzzing/README.md)
* [Phase 03: instrumenting native binaries with coverage tracking and address sanitization](./03-coverage_sanitized_fuzzing/README.md)

The `playground` directory contains a set of buggy binaries to play with along
with their source code. The tutorial expects that retrowrite is installed and
compiled in the `retrowrite` directory and the fuzzer in the `fuzzer` directory.
You can set up the necessary tools by executing `./setup.sh`.


## Contact

This tutorial is created by the HexHive group at EPFL and is first presented at
[SSSS'20](./https://www.cerias.purdue.edu/site/ssss20). In case of questions,
please contact [Mathias Payer](mailto:mathias.payer@epfl.ch), Jean-Michel
Crepel, or Antony Vennard.
