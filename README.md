# BForChapel
Currently named "chplBlamer",  BForChapel is a data-centric and code-centric combined performance charactorization, evaluation and analysis tool for Chapel language, in both single-locale and multi-locale environment.

It uses a novel data-centric profiling scheme called "blame analysis" to map the performance data back to source code variables instead of the functions as the traditional code-centric profilers did. Static analysis and runtime sampling are combined to generate data profiles. Postmortem analysis is used and a Graphic-User-Interface is provided.

The project also involves contribution to the Chapel LLVM backend and minimum instrumentation to the Chapel runtime library.
