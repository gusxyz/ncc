# self hosting novo

Most of the basic language is there now. I should be able to start porting the compiler once arrays work and I have a small runtime. Don't need to finish every language feature before starting.

## what works already

- recursive functions, forward calls and mutually recursive functions
- locals, globals, nested scopes, inference, constants and type aliases
- structs, alignment, nested structs, self referencing structs and struct pointers
- member access with `.` and `->`
- pointer arithmetic and pointer indexing, once the current integer regression is fixed
- u8/char storage, strings and character literals
- comparisons, short circuit logic, loops, break and continue
- imports and linux syscalls

Enough to write a lexer, parser, AST, symbol tables and assembly generator. Main things left are arrays, memory allocation and actually being able to run the compiler as a normal program.

## compiler features still missing

### sizeof

Need this for allocating compiler objects and arrays without hardcoding sizes everywhere.

### forward struct declarations

Self referencing structs work, mutually referencing structs don't. The compiler has types like `Node` and `Type` that need to refer to each other, so this is pretty high priority unless I rewrite those relationships to use integer handles.

Function forward references already work because signatures get collected before parsing bodies. Structs need a way to exist before their full definition too.

### consistent integer types

The extra integer kinds are declared but aren't implemented throughout the compiler. Loads/stores mostly only know about bytes and qwords. The u16/u32 placeholder sizes are also still 1.

`int` is currently tagged unsigned but comparisons and division generate signed instructions. Need to settle on a consistent initial integer model before adding every width.

Token values, AST numbers and constant evaluation still use C `int`. `4294967296` currently turns into `0`, so full width literals need fixing too.

### conversions and type checking

Assignments, arguments and returns don't really check type compatibility yet. A function expecting `int*` will accept `42`.

Need deliberate pointer conversions for things like allocator/syscall results, and checks that catch accidental mismatches while porting. The current permissiveness lets some code compile but will make bugs harder to find.

### struct copies

Whole struct values are explicitly unsupported right now. The C compiler does things like copying an `ImportRequest` out of an array, which won't port directly.

Can avoid needing this immediately by using pointers and assigning fields individually. Struct arguments/returns by value can wait too.

### initialized globals and aggregate initializers

Globals only get zeroed storage. No `int counter = 1;` or initialized arrays of structs yet.
The C lexer uses initialized operator and keyword tables. Can build those in an init function for now instead of implementing all initializer syntax first.

### more operators

Missing shifts and complement. Useful for integer formatting, hashing, alignment and syscall flags.
Not all strictly required to start, but worth adding as the port needs them.

## runtime work

These aren't all compiler syntax features, but the self hosted compiler needs them.

- [ ] expose command line arguments. `_start` currently calls `main` without setting up argc/argv
- [ ] arena allocator using mmap, plus typed allocation helpers
- [ ] source file loading and output writing
- [ ] string length, comparison and copying
- [ ] path handling for imports
- [ ] string/integer output for assembly and diagnostics

Don't need a full malloc/realloc implementation immediately. An arena should cover most compiler allocations. Dynamic arrays can grow by allocating a new buffer and copying into it.

The C emitter uses variadic printf everywhere, but novo doesn't need variadics just to replace that. Can use regular string/integer output functions.

Also need to fix import path resolution: it currently uses the first slash in the source path instead of the last, so deeper directory layouts resolve incorrectly.

## things that can wait

- generics, use separate append functions instead of the C `da_append` macro
- enums, constants work for now
- floating point
- function pointers
- optimization
- full C ABI support
- more than 6 function parameters, can pass a context pointer instead
- void functions, can return 0 for now
- struct returns and other aggregate value operations if the port sticks to pointers

Can keep using external `as` and `ld`. Self hosting doesn't require writing an assembler or linker.

## order
1. Get argc/argv and the small runtime working.
2. Start porting the lexer, then parser and codegen.
3. Add other features when the port actually needs them.

Actual milestone is the C compiler building the novo compiler, then that compiler rebuilding itself, then the rebuilt compiler passing the same tests.
