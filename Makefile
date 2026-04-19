.PHONY: all
all:
	@echo "Targets:"
	@echo "  make build    build firmware"
	@echo "  make upload   build + flash firmware"
	@echo "  make monitor  serial monitor"
	@echo "  make flash    upload + monitor"
	@echo "  make clean    clean build artifacts"
	@echo "  make format   clang-format src/ include/"

.PHONY: format
format:
	find ./src ./include \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;

.PHONY: build
build:
	pio run

.PHONY: upload
upload:
	pio run -t upload

.PHONY: monitor
monitor:
	pio device monitor

.PHONY: flash
flash: upload monitor

.PHONY: clean
clean:
	pio run -t clean
