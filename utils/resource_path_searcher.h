#pragma once

#ifndef FILESYSTEM_TS
#include <filesystem>
namespace fs = std::filesystem;
#else
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif

#include <string>
#include <set>
namespace DR {

    class ResourcePathSearcher {
    private:
#ifndef FILESYSTEM_TS
        using Path = std::filesystem::path;
#else
        using Path = std::experimental::filesystem::path;
#endif
        std::set<Path> search_paths_;

    public:
        static Path root_path;
        ResourcePathSearcher();
        ResourcePathSearcher(const ResourcePathSearcher &) = delete;
        ResourcePathSearcher &operator=(const ResourcePathSearcher &) = delete;
        ResourcePathSearcher(ResourcePathSearcher &&) = delete;
        void add_path(const std::string &path);
        void add_path(const Path &paths);
        // pass a vector such as {"src","cores","xxx.cc"}
        std::string find_path(const std::vector<std::string>& paths) const;
        std::string find_path(const std::string &filename) const;
    };
}// namespace DR
