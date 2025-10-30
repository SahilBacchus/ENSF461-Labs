.PHONY: all 
all: os_check gen_numbers.out gen_in_range.out select_arg.out test_gen_numbers test_gen_in_range test_select_arg

COMPILER = gcc
OS = $(shell uname)

os_check: 
ifneq ($(OS), Linux)
	@echo "Sorry, I prefer Linux"
endif

%.out: %.c utils.c
	$(COMPILER) -o $@ $^


test_gen_numbers:
	@./gen_numbers.out 10; \
	exit_code=$$?; \
	if [ $$exit_code -eq 0 ]; then \
		echo "gen_numbers passed"; \
	else \
		echo "fail"; \
	fi


test_gen_in_range:
	@./gen_in_range.out 10 20; \
	exit_code=$$?; \
	if [ $$exit_code -eq 0 ]; then \
		echo "gen_in_range passed"; \
	else \
		echo "fail"; \
	fi

test_select_arg:
	@./select_arg.out 10; \
	exit_code=$$?; \
	if [ $$exit_code -eq 0 ]; then \
		echo "select_arg passed"; \
	else \
		echo "fail"; \
	fi


.PHONY: clean
clean:
	rm -f *.out