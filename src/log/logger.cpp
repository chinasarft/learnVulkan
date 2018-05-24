#include "logger.h"
//https://github.com/gabime/spdlog/wiki/Using-printf-syntax

std::shared_ptr<spdlog::logger> spdlogger;

extern "C" void logger_init_file_output(const char * path) {
    if(spdlogger.get() != nullptr)
        return;
    
    char filepath[512] = { 0 };
    strcpy(filepath, path);
    
    spdlogger = spdlog::rotating_logger_mt("file_logger", filepath, 1024 * 1024 * 10, 3);
    if (spdlogger.get() != nullptr)
        spdlogger->info("ok");
}

extern "C" void logger_set_level_debug(){
    if(spdlogger.get() != nullptr)
        return;

    spdlogger->set_level(spdlog::level::level_enum::debug);
}

extern "C" void logger_set_level_info(){
    if(spdlogger.get() != nullptr)
        return;
    
    spdlogger->set_level(spdlog::level::level_enum::info);
}

extern "C" void logger_set_level_warn(){
    if(spdlogger.get() != nullptr)
        return;
    
    spdlogger->set_level(spdlog::level::level_enum::warn);
}

extern "C" void logger_set_level_error(){
    if(spdlogger.get() != nullptr)
        return;
    
    spdlogger->set_level(spdlog::level::level_enum::err);
}
