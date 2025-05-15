CXX = g++
CXXFLAGS = -Wall -fPIC -g $(shell pkg-config --cflags hexchat-plugin gtk+-3.0 gpgme)
LDFLAGS = -shared $(shell pkg-config --libs hexchat-plugin gtk+-3.0 gpgme)
PLUGIN_NAME = hexchat-gpg.so
SRC_DIR = src
BUILD_DIR = build
SOURCES = $(SRC_DIR)/hexchat-gpg.cpp $(SRC_DIR)/lru_cache.cpp $(SRC_DIR)/recipient_list.cpp $(SRC_DIR)/config_gui.cpp $(SRC_DIR)/directory_client.cpp
OBJECTS = $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(SOURCES))
PREFIX = ~/.local/share/hexchat/plugins

all: $(BUILD_DIR)/$(PLUGIN_NAME)

$(BUILD_DIR)/$(PLUGIN_NAME): $(OBJECTS)
	$(CXX) $(OBJECTS) -o $@ $(LDFLAGS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)

install: $(BUILD_DIR)/$(PLUGIN_NAME)
	mkdir -p $(PREFIX)
	cp $(BUILD_DIR)/$(PLUGIN_NAME) $(PREFIX)

.PHONY: all clean install