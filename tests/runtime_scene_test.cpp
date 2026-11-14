#include <chrono>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <string>

extern "C" int64_t npp_scene_create();
extern "C" bool npp_scene_save(int64_t scene, const char* path);
extern "C" int64_t npp_scene_load(const char* path);
extern "C" void npp_scene_destroy(int64_t scene);
extern "C" int64_t npp_ecs_create();
extern "C" void npp_ecs_destroy(int64_t id);
extern "C" void npp_ecs_set_position(int64_t id, double x, double y);
extern "C" void npp_ecs_set_velocity(int64_t id, double x, double y);
extern "C" void npp_ecs_set_size(int64_t id, double w, double h);
extern "C" void npp_scene_add(int64_t scene, int64_t entity);

namespace fs = std::filesystem;

static std::string readAll(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>());
}

int main() {
    const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
    const fs::path directory =
        fs::temp_directory_path() / ("npp-scene-save-test-" + std::to_string(stamp));
    std::error_code ec;
    fs::create_directories(directory, ec);
    if (ec)
        return 1;

    const fs::path destination = directory / "scene.nxs";
    const fs::path roundTrip = directory / "round-trip.nxs";
    const int64_t scene = npp_scene_create();
    const int64_t entity = npp_ecs_create();
    npp_ecs_set_position(entity, 12.5, -3.25);
    npp_ecs_set_velocity(entity, 4.0, 8.0);
    npp_ecs_set_size(entity, 64.0, 32.0);
    npp_scene_add(scene, entity);

    int result = 0;
    {
        std::ofstream old(destination, std::ios::binary);
        old << "previous save";
        if (!old)
            result = 2;
    }
    if (result == 0 && !npp_scene_save(scene, destination.string().c_str()))
        result = 3;

    const std::string saved = readAll(destination);
    if (result == 0 && saved.find("NPP_SCENE 1\n1\n") != 0)
        result = 4;
    if (result == 0 && saved.find("\r\n") != std::string::npos)
        result = 14;

    // A directory cannot be replaced by a scene file. The failed save must
    // leave the directory intact and clean up its temporary sibling.
    if (result == 0 && npp_scene_save(scene, directory.string().c_str()))
        result = 5;
    if (result == 0 && !fs::is_directory(directory))
        result = 6;
    if (result == 0) {
        for (const auto& entry : fs::directory_iterator(directory)) {
            if (entry.path().filename().string().find(".npp-tmp-") != std::string::npos) {
                result = 7;
                break;
            }
        }
    }

    const int64_t loaded = result == 0 ? npp_scene_load(destination.string().c_str()) : 0;
    if (result == 0 && loaded == 0)
        result = 8;
    if (result == 0 && !npp_scene_save(loaded, roundTrip.string().c_str()))
        result = 9;
    if (result == 0 && readAll(roundTrip) != saved)
        result = 10;

    // Invalid scene handles must not modify an existing good save.
    if (result == 0 && npp_scene_save(-1, destination.string().c_str()))
        result = 11;
    if (result == 0 && readAll(destination) != saved)
        result = 12;

    npp_scene_destroy(scene);
    if (loaded != 0)
        npp_scene_destroy(loaded);
    npp_ecs_destroy(entity);
    fs::remove_all(directory, ec);
    if (result == 0 && ec)
        result = 13;

    if (result != 0) {
        std::fprintf(stderr, "NPP scene save test failed at check %d\n", result);
        return result;
    }
    std::puts("NPP scene save smoke: PASS");
    return 0;
}
