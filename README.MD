# novolang compiler

I decide I wanted to build out my own self-hosted langauge so heres my attempt at that, currently in the bootstrapping stage of writing it in C till I can support enough features in my own language to self-host.

First shot at a compiler or writing a proper language.

Run tests from the project root or from `tests/`:

```sh
make test                       # all suites
make test-list                  # available suites
make test-pointers              # pointer suite
make test-imports               # import suite
make test TEST="pointers:3 functions" # one case and another suite
```