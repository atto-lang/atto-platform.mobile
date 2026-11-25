.PHONY: test
CC ?= cc
test:
	@mkdir -p .build
	$(CC) -std=c11 -Wall -Wextra -Werror -pedantic shared/src/atto_mobile_ui_v1.c shared/tests/admission_test.c -o .build/mobile-admission
	./.build/mobile-admission
