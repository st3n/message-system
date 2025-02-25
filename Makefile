BUILD_DIR = build

# Default target: configure, build
all: configure build

configure:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake -DCMAKE_BUILD_TYPE=Release ..

build: configure
	cd $(BUILD_DIR) && $(MAKE) all

run: build
	$(BUILD_DIR)/message-system/NetworkProcessorApp 5001 5002 5003

clean:
	rm -rf $(BUILD_DIR)
