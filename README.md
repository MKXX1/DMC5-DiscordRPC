Discord Rich Presence for Devil May Cry 5
==========================================

A REFramework plugin that brings your Devil May Cry 5 session to Discord.
Show current Style ranks in the campaign or pushing through
Bloody Palace, your status stays in sync with what's on screen.

[images/1.jpg] [images/2.jpg] [images/3.jpg]


Features
--------

- Character display: shows Nero, Dante, V, or Vergil as large-image text
- Style rank tracker: small badge that updates live (D -> SSS) in combat
- Mission info: displays the current story mission name
- Bloody Palace support: shows the floor number you're on
- Multi-language support: 13 locales included, with automatic detection


Installation
------------

1. Install REFramework for DMC5 if you haven't already:
   - [Nexus](https://www.nexusmods.com/devilmaycry5/mods/2760)
   - [GitHub](https://github.com/praydog/REFramework/releases)

2. Grab the latest release of this plugin:
   - [Releases](https://github.com/MKXX1/DMC5-DiscordRPC/releases)

3. Drop both files into reframework/plugins/ inside your DMC5 folder:
   - DMC5_DiscordRPC.dll
   - locrpc.json

4. Launch the game. Your Discord status should update on its own.

NOTE: locrpc.json must be placed alongside the DLL. If it's
missing, some strings will be missing.


Building from Source
--------------------

Requirements:
- Visual Studio 2019/2022 with the "Desktop development with C++" workload
- CMake 3.15 or newer
- Git

Dependencies to place in the repository root:

    reframework/include/reframework/API.hpp   - REFramework SDK
    discord-rpc/win64-static/                 - Discord RPC static library
    plugin/json.hpp                           - nlohmann/json (single header)

Build steps:

    git clone <https://github.com/MKXX1/DMC5-DiscordRPC.git>
    cd <DMC5-DiscordRPC>
    mkdir build && cd build
    cmake ..
    cmake --build . --config Release

The build outputs DMC5_DiscordRPC.dll and locrpc.json into
build/Release/. Copy both files into reframework/plugins/.

You can override dependency paths via CMake flags if your SDKs live
elsewhere:

    cmake .. \
        -DREF_SDK_DIR="D:/SDKs/reframework" \
        -DDISCORD_RPC_DIR="D:/SDKs/discord-rpc/win64-static"


Localization
------------

The plugin reads the current game language through via.ResourceManager
and picks the matching block from locrpc.json. Out of the box,
these languages are supported:

    en     - English            ko     - 한국어
    ru     - Русский            zh-CN  - 简体中文
    ja     - 日本語              zh-TW  - 繁體中文
    fr     - Français           pt     - Português
    it     - Italiano           pt-BR  - Português (Brasil)
    de     - Deutsch            pl     - Polski
    es     - Español

If your language isn't listed, English is used instead.

### Creating a custom translation

The localization file has a simple structure. Each language block
contains four sections: players, missions, ranks, and fallback.

Here's a minimal example for Czech (cs):

    {
      "cs": {
        "players": {
          "-1": "Žádný",
          "0": "Nero",
          "1": "Dante",
          "2": "V",
          "3": "Vergil",
          "4": "Vergil"
        },
        "missions": {
          "0": "PROLOG",
          "1": "Nero",
          "2": "Qlifoth",
          "3": "Létající lovec"
        },
        "ranks": {
          "0": "-",
          "1": "Dismal",
          "2": "Crazy",
          "3": "Badass",
          "4": "Apocalyptic",
          "5": "Savage",
          "6": "Sick Skills",
          "7": "Smokin' Sexy Style"
        },
        "fallback": {
          "details": "Devil May Cry 5",
          "large_text": "Devil May Cry 5",
          "state_battle": "V boji",
          "state_explore": "Průzkum",
          "bloody_palace": "Bloody Palace - Patro {floor}"
        }
      }
    }

Step by step:

1. Open locrpc.json in any text editor.
2. Copy the entire "en" block.
3. Rename the copied key to your language code (e.g. "cs", "tr", "uk").
4. Translate the strings inside. Keep the numeric keys unchanged.
5. Save the file with UTF-8 encoding.
6. Launch the game and check the log file. You should see:

       Localization loaded: cs

If something goes wrong, the plugin logs a warning and falls back to
English, so the game keeps working.

Fields explained:

- players     -> names shown for PlayingID values (0 = Nero, 1 = Dante,
                 2 = V, 3 and 4 = Vergil)
- missions    -> titles for each mission index (0 = prologue, 1-20 =
                 story missions)
- ranks       -> style rank names (only text; the small image key is
                 always d/c/b/a/s/ss/sss)
- fallback    -> generic strings used outside combat or when a value is
                 unavailable. "{floor}" in bloody_palace is replaced
                 with the current floor number at runtime.

The translation file is reloaded each time the game starts, so no
rebuild is required to test your changes.


Troubleshooting
---------------

Status doesn't show up
    - Make sure the Discord desktop app is running.
    - Check reframework_log.txt for "Discord: Ready" and
      "Localization loaded" lines.
    - Try restarting the game; Discord sometimes needs a nudge on the
      first launch.

Wrong language displayed
    - Look for "Game language ID: N -> code: XX" in the log.
    - If the code is unexpected, verify that your locrpc.json has
      a section for that code, or edit the log line to hardcode it.

Crash on game exit
    - If you consistently get a crash when closing DMC5, remove the
      Discord_Shutdown() call from DllMain. Discord will close the
      connection on its own after a timeout.


Licenses
-------
[Discord-RPC](https://github.com/discord/discord-rpc)

[REFramework](https://github.com/praydog/REFramework)

[Json](https://github.com/nlohmann/json)