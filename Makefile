.PHONY: all
all:
	@echo "Targets:"
	@echo "  make build    build firmware (release)"
	@echo "  make upload   build + flash firmware (release)"
	@echo "  make build-debug / upload-debug / flash-debug   same with debug build"
	@echo "  make monitor  serial monitor"
	@echo "  make flash    upload + monitor"
	@echo "  make clean    clean build artifacts"
	@echo "  make format   clang-format src/ include/"
	@echo "  make ui       lint, test, build device web UI into data/public_html"

.PHONY: format
format:
	find ./src ./include \( -name "*.c" -o -name "*.h" \) -exec clang-format -i {} \;

.PHONY: build
build:
	pio run

.PHONY: upload
upload:
	pio run -t upload

DEBUG_ENV = esp32-c3-devkitm-1-debug

.PHONY: build-debug
build-debug:
	pio run -e $(DEBUG_ENV)

.PHONY: upload-debug
upload-debug:
	pio run -e $(DEBUG_ENV) -t upload

.PHONY: flash-debug
flash-debug: upload-debug monitor

.PHONY: monitor
monitor:
	pio device monitor

.PHONY: flash
flash: upload monitor

.PHONY: clean
clean:
	pio run -t clean

.PHONY: ui
ui:
	cd ui && pnpm install --frozen-lockfile && pnpm run lint && pnpm run fmt:check && pnpm run test && pnpm run build
