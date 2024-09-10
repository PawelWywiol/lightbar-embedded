.PHONY: all
all:
	@echo "Use 'make format' to format the code."

.PHONY: format
format:
	find ./src ./include \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;
