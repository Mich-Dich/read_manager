
#include "util/crash_handler.h"
#include "util/io/logger.h"
#include "application.h"


int main(int argc, char *argv[]) {

    ASSERT_SS(logger_init("[$B$T.$J $L$E][$B$Q $I $F:$G$E] $C", true, "logs", "application", false))            // logger should be external to application
    LOGGER_REGISTER_THREAD_LABEL("main")
    ASSERT_SS(crash_handler_init())
    crash_handler_subscribe_callback(logger_shutdown);

    
    VALIDATE(application_init(argc, argv), logger_shutdown(); return 1, "", "Failed to init the application")
    application_run();
    application_shutdown();

    crash_handler_shutdown();
    logger_shutdown();
    return 0;
}
