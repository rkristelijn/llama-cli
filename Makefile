BUILD_DIR = build

.PHONY: all clean run

all:
	cmake -B $(BUILD_DIR) -S .
	cmake --build $(BUILD_DIR)

run: all
	./$(BUILD_DIR)/llama-cli

clean:
	rm -rf $(BUILD_DIR)
