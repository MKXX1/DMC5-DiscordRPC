#include <Windows.h>
#include "reframework/API.hpp"
#include <discord_rpc.h>
#include <string>
#include <ctime>
#include <fstream>
#include <vector>
#include <utility>
#include "json.hpp"

using nlohmann::json;
using namespace reframework;

static DiscordRichPresence g_presence = {};
static int64_t g_start_timestamp = 0;

static json        g_loc;                      
static std::string g_lang_code = "en";          
static std::string g_plugin_dir;                

static const char* rank_to_letter(int rank) {
    switch (rank) {
    case 0: return "-";
    case 1: return "d";
    case 2: return "c";
    case 3: return "b";
    case 4: return "a";
    case 5: return "s";
    case 6: return "ss";
    case 7: return "sss";
    default: return "?";
    }
}

#pragma region Localization

static int read_game_language() {
    auto api = API::get().get();

    auto rm = api->get_native_singleton("via.ResourceManager");
    if (rm == nullptr) return -1;

    if (auto m = api->tdb()->find_method("via.ResourceManager", "get_Language")) {
        return m->call<int>(api->get_vm_context(), rm);
    }

    return -1;
}

static std::string loc_format(const std::string& tmpl,
    const std::vector<std::pair<std::string, std::string>>& vars) {
    std::string out = tmpl;
    for (const auto& kv : vars) {
        const std::string token = "{" + kv.first + "}";
        size_t pos = 0;
        while ((pos = out.find(token, pos)) != std::string::npos) {
            out.replace(pos, token.size(), kv.second);
            pos += kv.second.size();
        }
    }
    return out;
}

static std::string loc_get(const char* section, const std::string& key) {
    try {
        if (!g_loc.is_object()) return {};
        if (!g_loc.contains(section)) return {};
        const auto& s = g_loc[section];
        if (!s.is_object()) return {};
        if (!s.contains(key)) return {};
        return s[key].get<std::string>();
    }
    catch (...) {
        return {};
    }
}

static std::string loc_player(int id) {
    std::string v = loc_get("players", std::to_string(id));
    return v.empty() ? "?" : v;
}

static std::string loc_mission(int id) {
    std::string v = loc_get("missions", std::to_string(id));
    return v.empty() ? "?" : v;
}

static std::string loc_rank(int id) {
    std::string v = loc_get("ranks", std::to_string(id));
    return v.empty() ? "?" : v;
}

static std::string loc_fallback(const char* key) {
    std::string v = loc_get("fallback", key);
    return v.empty() ? std::string(key) : v;
}

static bool load_localization(const std::string& lang_code) {
    if (g_plugin_dir.empty()) {
        API::get()->log_error("Localization: plugin dir is empty, skipping load");
        return false;
    }

    std::string path = g_plugin_dir + "\\locrpc.json";

    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        API::get()->log_warn("Localization file not found: %s", path.c_str());
        return false;
    }

    try {
        json root = json::parse(f);

        std::string use_lang = lang_code;

        if (!root.contains(use_lang)) {
            auto dash = use_lang.find('-');
            if (dash != std::string::npos) {
                std::string base = use_lang.substr(0, dash);
                if (root.contains(base)) {
                    API::get()->log_info("Using base language '%s' for '%s'",
                        base.c_str(), use_lang.c_str());
                    use_lang = base;
                }
            }
        }

        if (!root.contains(use_lang)) {
            API::get()->log_warn("Language '%s' not found, fallback to 'en'",
                use_lang.c_str());
            use_lang = "en";
            if (!root.contains(use_lang)) {
                API::get()->log_error("No 'en' fallback in localization.json");
                return false;
            }
        }

        g_loc = root[use_lang];
        g_lang_code = use_lang;

        API::get()->log_info("Localization loaded: %s", g_lang_code.c_str());
        return true;
    }
    catch (const std::exception& e) {
        API::get()->log_error("Failed to parse localization.json: %s", e.what());
        return false;
    }
}

static std::string language_code_from_id(int id) {
    switch (id) {
    case 0:  return "ja";
    case 1:  return "en";
    case 2:  return "fr";
    case 3:  return "it";
    case 4:  return "de";
    case 5:  return "es";
    case 6:  return "ru";
    case 7:  return "pl";
    case 8:  return "nl";
    case 9:  return "pt";
    case 10: return "pt-BR";
    case 11: return "ko";
    case 12: return "zh-TW";  
    case 13: return "zh-CN"; 
    case 14: return "fi";
    case 15: return "sv";
    case 16: return "da";
    case 17: return "no";
    case 18: return "cs";
    case 19: return "hu";
    case 20: return "sk";
    case 21: return "ar";
    case 22: return "tr";
    case 23: return "bg";
    case 24: return "el";
    case 25: return "ro";
    case 26: return "th";
    case 27: return "uk";
    default: return "en";
    }
}

#pragma endregion

static void handle_discord_ready(const DiscordUser* user) {
    API::get()->log_info("Discord: Ready (%s)", user->username);
}
static void handle_discord_disconnected(int errorCode, const char* message) {
    API::get()->log_warn("Discord: Disconnected (%d: %s)", errorCode, message);
}
static void handle_discord_errored(int errorCode, const char* message) {
    API::get()->log_error("Discord: Error (%d: %s)", errorCode, message);
}

#pragma region Gameread 

static int read_style_rank() {
    auto api = API::get().get();
    auto sm = api->get_managed_singleton("app.StylishManager");
    if (sm == nullptr) return 0;

    auto method = api->tdb()->find_method("app.StylishManager", "get_stylishRank");
    if (method == nullptr) return 0;

    return method->call<int>(api->get_vm_context(), sm);
}

static bool is_in_battle() {
    auto api = API::get().get();
    auto sm = api->get_managed_singleton("app.StylishManager");
    if (sm == nullptr) return false;

    auto method = api->tdb()->find_method("app.StylishManager", "get_isBattle");
    if (method == nullptr) return false;

    return method->call<bool>(api->get_vm_context(), sm);
}

static std::string read_GAMERPC() {
    auto api = API::get().get();
    auto gm = api->get_managed_singleton("app.GameManager");
    if (gm == nullptr) return "---";

    auto method = api->tdb()->find_method("app.GameManager", "getRichPresenceText");
    if (method == nullptr) return "---";

    uintptr_t raw = method->call<uintptr_t>(api->get_vm_context(), gm);
    if (raw == 0) return "---";

    uint8_t* p = reinterpret_cast<uint8_t*>(raw);

    constexpr int LEN_OFF = 0x10;
    constexpr int STR_OFF = 0x14;

    int32_t length = *reinterpret_cast<int32_t*>(p + LEN_OFF);
    if (length <= 0 || length > 4096) return "";

    const wchar_t* wchars = reinterpret_cast<const wchar_t*>(p + STR_OFF);

    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, wchars, length,
        nullptr, 0, nullptr, nullptr);
    if (utf8_len <= 0) return "";

    std::string result(utf8_len, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wchars, length,
        &result[0], utf8_len, nullptr, nullptr);
    return result;
}

static int read_playerID() {
    auto api = API::get().get();
    auto gm = api->get_managed_singleton("app.GameManager");
    if (gm == nullptr) return -1;

    auto field = api->tdb()->find_field("app.GameManager", "PlayingID");
    if (field == nullptr) return -1;

    return field->get_data<int32_t>(gm);
}

static bool read_IsMissionPlaying() {
    auto api = API::get().get();
    auto msm = api->get_managed_singleton("app.MissionSettingManager");
    if (msm == nullptr) return false;

    auto method = api->tdb()->find_method("app.MissionSettingManager", "get_IsMissionPlaying");
    if (method == nullptr) return false;

    return method->call<bool>(api->get_vm_context(), msm);
}

static bool read_IsBPPlaying() {
    auto api = API::get().get();
    auto msm = api->get_managed_singleton("app.MissionSettingManager");
    if (msm == nullptr) return false;

    auto method = api->tdb()->find_method("app.MissionSettingManager", "get_isBloodyPalaceEntered");
    if (method == nullptr) return false;

    return method->call<bool>(api->get_vm_context(), msm);
}

static int read_missionNo() {
    auto api = API::get().get();
    auto gm = api->get_managed_singleton("app.GameManager");
    if (gm == nullptr) return -1;

    auto method = api->tdb()->find_method("app.GameManager", "get_missionNo");
    if (method == nullptr) return -1;

    return method->call<int>(api->get_vm_context(), gm);
}

static int read_BPStageNo() {
    auto api = API::get().get();
    auto bpm = api->get_managed_singleton("app.BloodyPalaceManager");
    if (bpm == nullptr) return -1;

    auto method = api->tdb()->find_method("app.BloodyPalaceManager", "get_currentStageNo");
    if (method == nullptr) return -1;

    return method->call<int>(api->get_vm_context(), bpm);
}

#pragma endregion

#pragma region Discord Presence Update

static std::string fix_min_length(const std::string& s) {
    if (s.empty()) return s;
    if (s.size() >= 2) return s;
    return s + "\xE2\x80\x8B"; 
}

static void update_presence() {
    int rank_idx = read_style_rank();
    std::string gamerpc = read_GAMERPC();
    int missionNo = read_missionNo();
    int BPStageNo = read_BPStageNo();

    bool battle = is_in_battle();
    bool isBP = read_IsBPPlaying();
    bool isMission = read_IsMissionPlaying();

    int pid = read_playerID();

    // Rank Small image
    std::string small_key, small_text;
    if (rank_idx > 0) {
        small_key = rank_to_letter(rank_idx);
        small_text = loc_rank(rank_idx);
    }

	// Details, curMission or curBPNo
    std::string details;
    if (isBP) {
        details = loc_format(
            loc_fallback("bloody_palace"),
            { {"floor", std::to_string(BPStageNo)} }
        );
    }
    else if (isMission) {
        details = loc_mission(missionNo);
    }
    else {
        details = "";
    }

    if (details == "?") {
        details.clear();
    }
    


    // LargeImage
    const std::string large_key = "dmc5";
    std::string large_text = loc_player(pid);
    if (large_text.empty() || large_text == "?") {
        large_text = loc_fallback("large_text");
    }

    // State,RPC from game
    std::string state = gamerpc.empty()
        ? (battle ? loc_fallback("state_battle") : loc_fallback("state_explore"))
        : gamerpc;

    details = fix_min_length(details);
    state = fix_min_length(state);

    static std::string cached_details, cached_state;
    static std::string cached_smallkey, cached_smalltext;
    static std::string cached_largekey, cached_largetext;

    if (details == cached_details &&
        state == cached_state &&
        small_key == cached_smallkey &&
        small_text == cached_smalltext &&
        large_key == cached_largekey &&
        large_text == cached_largetext) return;

    cached_details = details;
    cached_state = state;
    cached_smallkey = small_key;
    cached_smalltext = small_text;
    cached_largekey = large_key;
    cached_largetext = large_text;

    g_presence.details = cached_details.empty() ? nullptr : cached_details.c_str();
    g_presence.state = cached_state.empty() ? nullptr : cached_state.c_str();
    g_presence.startTimestamp = g_start_timestamp;

    g_presence.smallImageKey = cached_smallkey.empty() ? nullptr : cached_smallkey.c_str();
    g_presence.smallImageText = cached_smalltext.empty() ? nullptr : cached_smalltext.c_str();

    g_presence.largeImageKey = cached_largekey.empty() ? nullptr : cached_largekey.c_str();
    g_presence.largeImageText = cached_largetext.empty() ? nullptr : cached_largetext.c_str();

    Discord_UpdatePresence(&g_presence);
}

static void on_present() {
    update_presence();
    Discord_RunCallbacks();
}

#pragma endregion

#pragma region REFramework

extern "C" __declspec(dllexport) void reframework_plugin_required_version(REFrameworkPluginVersion* version) {
    version->major = REFRAMEWORK_PLUGIN_VERSION_MAJOR;
    version->minor = REFRAMEWORK_PLUGIN_VERSION_MINOR;
    version->patch = REFRAMEWORK_PLUGIN_VERSION_PATCH;
}

extern "C" __declspec(dllexport) bool reframework_plugin_initialize(const REFrameworkPluginInitializeParam* param) {
    API::initialize(param);
    g_start_timestamp = std::time(nullptr);

    {
        HMODULE hmod = nullptr;
        GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(&reframework_plugin_initialize),
            &hmod);

        char path[MAX_PATH] = {};
        GetModuleFileNameA(hmod, path, MAX_PATH);

        std::string full = path;
        auto pos = full.find_last_of("\\/");
        g_plugin_dir = (pos != std::string::npos) ? full.substr(0, pos) : ".";

        API::get()->log_info("Plugin dir: %s", g_plugin_dir.c_str());
    }

    {
        int lang_id = read_game_language();
        std::string lang_code = language_code_from_id(lang_id);
        API::get()->log_info("Game language ID: %d -> code: %s",
            lang_id, lang_code.c_str());

        if (!load_localization(lang_code)) {
            API::get()->log_warn("Falling back to 'en'");
            load_localization("en");
        }
    }

    DiscordEventHandlers handlers = {};
    handlers.ready = handle_discord_ready;
    handlers.disconnected = handle_discord_disconnected;
    handlers.errored = handle_discord_errored;

    const char* application_id = "1555001889408684152";
    Discord_Initialize(application_id, &handlers, 1, nullptr);

    if (param->functions && param->functions->on_present) {
        param->functions->on_present(on_present);
    }

    API::get()->log_info("DMC5 Discord RPC plugin initialized.");
    return true;
}

#pragma endregion

BOOL APIENTRY DllMain(HMODULE hModule, DWORD ul_reason_for_call, LPVOID lpReserved) {
    if (ul_reason_for_call == DLL_PROCESS_DETACH) {
        Discord_Shutdown();
    }
    return TRUE;
}