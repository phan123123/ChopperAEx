# ChopperAEx
This is a dynamic plugin for `opt` - LLVM optimizer for choosing excluded funtions automatically for Chopper in patch testing.
Target function, the function is modified or updated, is needed to determine.

## Idea
we using [basic block CFG](https://en.wikipedia.org/wiki/Control-flow_graph) to find the paths to target function and try exclude others.
Before add a function to exclude list, we will check [Cyclomatic complexity](https://en.wikipedia.org/wiki/Cyclomatic_complexity), so it make result of Chopper more balanced between good case and bad case.

## Installation
`mkdir build; cd build`

`cmake ..`

`make`

This will build the `ChopperAEx.so` file that can then be invoked with `opt`.

## Usage
This pass is intended to be used with target functions provided by [diffanalyze](https://github.com/davidbuterez/diffanalyze). A typical use scenario looks like this:

`opt -load <path-to-ChopperAEx.so> -exclude -target <target-function-name> -file <assembly.bc> -disable-output <bitcode>`

The arguments are:
- `target` - the name of the target function
- `file` - to avoid some errors in Chopper, we require information from a previous run of KLEE or Chopper. Simply run KLEE/Chopper on the desired bitcode, and in the `klee-last` directory, KLEE will output an `assembly.ll` file, which can be turned to `.bc` with `llvm-as`. Alternatively, KLEE can output directly a bitcode file. Provide the `.bc` file as the argument.
You also use `.bc` file like `bitcode` below.
- `bitcode` - this is the bitcode file on which you intend to run KLEE/Chopper.

## Referents
We refer the structor of project from [urop-chopped-symbolic-execution](https://github.com/davidbuterez/urop-chopped-symbolic-execution)

