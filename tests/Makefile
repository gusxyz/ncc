# Forward test commands to the project Makefile, which builds the compiler first.
.DEFAULT_GOAL := test

TEST_SUITES := $(basename $(notdir $(wildcard cases/*.sh)))
TEST_TARGETS := test test-list $(addprefix test-,$(TEST_SUITES))

$(TEST_TARGETS):
	$(MAKE) -C .. $@

.PHONY: $(TEST_TARGETS)
