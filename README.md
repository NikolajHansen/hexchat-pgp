HexChat GPG Plugin
A HexChat plugin for secure IRC communication using GPG encryption.
Description
The hexchat-gpg plugin enables secure messaging in HexChat by encrypting messages with GPG using the GPGME library. It supports a configuration GUI, an LRU cache for public keys, recipient management, and an encryption toggle. Messages are encrypted for specified recipients and decrypted if they start with HEXCHAT_GPG. When encryption is disabled, the plugin reverts to vanilla HexChat behavior.
Features

Command Prefix: /GPG for all plugin commands.
LRU Cache: Stores up to 100 email-to-public-key mappings, with automatic lookup from a keyserver (e.g., keys.openpgp.org) on cache miss.
Configuration GUI: GTK-based interface to set your email, public key, and private key path.
Private Key Loading: Loads private key from a user-specified file path for decryption.
Message Decryption: Decrypts messages starting with HEXCHAT_GPG.
Recipient Management: Maintains a list of recipients based on IRC channel userlists.
Encryption Toggle: Enable/disable encryption with /GPG on and /GPG off.
Commands:
/GPG config: Opens the configuration GUI.
/GPG addrecipient <nick> <email>: Adds a recipient.
/GPG deluser <nick>: Removes a recipient.
/GPG listusers: Lists all recipients.
/GPG on: Enables encryption functionality.
/GPG off: Disables encryption, reverting to vanilla HexChat behavior.


Configuration Persistence: Stores settings in ~/.config/hexchat/hexchat-gpg.conf.
Selective Messaging: When encryption is on, messages are only sent to recipients in the recipient list.
GPGME Integration: Uses GPGME for keyserver lookups, encryption, and decryption.

Installation
Prerequisites

HexChat
GPGME library (libgpgme-dev)
GTK 3 (libgtk-3-dev)
HexChat development headers (hexchat-dev)
A C++ compiler (g++)
pkg-config
A GPG keyserver (e.g., keys.openpgp.org) accessible

On Ubuntu 24.04, install the prerequisites:
sudo apt update
sudo apt install hexchat-dev libgtk-3-dev libgpgme-dev g++ pkg-config gnupg

If pkg-config cannot find hexchat-plugin, gtk+-3.0, or gpgme, set the PKG_CONFIG_PATH:
export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH

Add to ~/.bashrc for persistence:
echo 'export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:/usr/share/pkgconfig:$PKG_CONFIG_PATH' >> ~/.bashrc
source ~/.bashrc

Verify dependencies:
pkg-config --modversion hexchat-plugin gtk+-3.0 gpgme

If hexchat-plugin.pc is missing, reinstall hexchat-dev:
sudo apt install --reinstall hexchat-dev

Build Instructions

Clone the repository:
git clone https://github.com/yourusername/hexchat-gpg.git
cd hexchat-gpg


Build the plugin:
make

This compiles sources from src/ and outputs to build/.

Install the plugin:
make install

This copies build/hexchat-gpg.so to ~/.local/share/hexchat/plugins.

Load the plugin in HexChat:

Go to Settings -> Plugins and Scripts.
Click Load and select hexchat-gpg.so.



Usage

Configure your GPG settings:
/GPG config

Enter your email, public key (in ASCII armor format), and private key path in the GUI.

Add recipients:
/GPG addrecipient <nick> <email>


List recipients:
/GPG listusers


Remove a recipient:
/GPG deluser <nick>


Enable encryption:
/GPG on


Disable encryption:
/GPG off



When encryption is enabled, messages are sent only to recipients in the recipient list, encrypted using their public keys. Received messages starting with HEXCHAT_GPG are decrypted automatically. When encryption is disabled, the plugin does not interfere with HexChat's normal operation.
Notes

Ensure your private key file is secure and accessible only to your user.
The plugin uses keys.openpgp.org as the default keyserver. Configure GPGME to use a different keyserver if needed (e.g., via gpgme.conf).
GPGME requires a properly configured GPG environment. Ensure GPG is installed (gnupg).
If compilation fails, check for missing .pc files in /usr/lib/x86_64-linux-gnu/pkgconfig or include paths. The Makefile includes fallbacks for common paths.

Project Structure

src/: Source files (hexchat-gpg.cpp, lru_cache.cpp, recipient_list.cpp, config_gui.cpp, directory_client.cpp, and headers).
build/: Compiled object files and the final hexchat-gpg.so plugin.
Makefile: Build configuration.
README.md: This file.

License
MIT License