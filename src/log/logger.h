#include <spdlog/spdlog.h>
#include <stdarg.h>
extern std::shared_ptr<spdlog::logger> spdlogger;
#define _s_l_(x) #x
#define _str_line_(x) _s_l_(x)
#define __STR_LINE__ _str_line_(__LINE__)

#define logdebug(...) \
    if(spdlogger.get() != nullptr) spdlogger->debug(__FILE__ ":" __STR_LINE__ ": " ##__VA_ARGS__)
#define loginfo(...) \
    if(spdlogger.get() != nullptr)  spdlogger->info(__FILE__ ":" __STR_LINE__ ": " ##__VA_ARGS__)
#define logwarn(...) \
    if(spdlogger.get() != nullptr)  spdlogger->warn(__FILE__ ":" __STR_LINE__ ": " ##__VA_ARGS__)
#define logerror(...) \
    if(spdlogger.get() != nullptr) spdlogger->error(__FILE__ ":" __STR_LINE__ ": " ##__VA_ARGS__)


extern "C" void logger_init_file_output(const char * path);
extern "C" void logger_set_level_debug();
extern "C" void logger_set_level_info();
extern "C" void logger_set_level_warn();
extern "C" void logger_set_level_error();

