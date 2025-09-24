

// #define TEST_YAML

#include "util/crash_handler.h"
#include "util/io/logger.h"
#include "application.h"

#if defined(TEST_YAML)
    #include <string.h>
    #include "util/io/serializer_yaml.h"
    #include "util/system.h"
    #include "util/data_structure/darray.h"


    typedef struct {
        char                string[512];
        i8                  int_8;
        u16                 uint_16;
        u8                  uint_8;                          // from 0 to 10
    } loop_test_struct;

    bool loop_test_struct_serializer_cb(SY* serializer, void* element) {

        loop_test_struct* local = (loop_test_struct*)element;
        sy_entry_str(serializer, S_KEY_VALUE(*local->string), sizeof(local->string));
        sy_entry(serializer, S_KEY_VALUE_FORMAT(local->int_8));
        sy_entry(serializer, S_KEY_VALUE_FORMAT(local->uint_16));
        sy_entry(serializer, S_KEY_VALUE_FORMAT(local->uint_8));
        return true;
    }

#endif


int main(int argc, char *argv[]) {

    ASSERT_SS(logger_init("[$B$T.$J $L$E][$B$Q $I $F:$G$E] $C", true, "logs", "application", false))            // logger should be external to application
    LOGGER_REGISTER_THREAD_LABEL("main")
    ASSERT_SS(crash_handler_init())
    crash_handler_subscribe_callback(logger_shutdown);

#if defined(TEST_YAML)

    char exec_path[PATH_MAX] = {0};
    get_executable_path_buf(exec_path, sizeof(exec_path));
    LOG(Trace, "exec_path: %s", exec_path)

    char loc_file_path[PATH_MAX];
    memset(loc_file_path, '\0', sizeof(loc_file_path));
    const int written = snprintf(loc_file_path, sizeof(loc_file_path), "%s/%s", exec_path, "config");
    VALIDATE(written >= 0 && (size_t)written < sizeof(loc_file_path), return false, "", "Path too long: %s/%s\n", exec_path, "config");


    i32 test_i32 = 200;
    b32 test_bool = true;
    f128 test_long_long = 5555;
    f32 test_f32 = 404.5050;
    f32 test_f32_s = 666.5050;
    darray loop_test_struct_array = {0};
    darray_init(&loop_test_struct_array, sizeof(loop_test_struct_array));
    
    // Create some test structs
    loop_test_struct test1 = {"Hello World", -10, 1000, 5};
    loop_test_struct test2 = {"Test String", 50, 2000, 8};
    loop_test_struct test3 = {"Another Test", -128, 3000, 2};
    
    // Push elements
    darray_push_back(&loop_test_struct_array, &test1);
    darray_push_back(&loop_test_struct_array, &test2);
    darray_push_back(&loop_test_struct_array, &test3);

    SY sy;
    ASSERT(sy_init(&sy, loc_file_path, "test.yml", "main_section", SERIALIZER_OPTION_SAVE), "", "");

    sy_entry(&sy, S_KEY_VALUE_FORMAT(test_i32));
    sy_entry(&sy, S_KEY_VALUE_FORMAT(test_f32));
    sy_entry(&sy, S_KEY_VALUE_FORMAT(test_bool));
    sy_entry(&sy, S_KEY_VALUE_FORMAT(test_long_long));
    // sy_loop(&sy, S_KEY_VALUE(loop_test_struct_array), sizeof(loop_test_struct), loop_test_struct_serializer_cb, 
    //     (sy_loop_callback_at_t)darray_get,
    //     (sy_loop_callback_append_t)darray_push_back,
    //     (sy_loop_DS_size_callback_t)darray_size);
    
    #define USE_SUB_SECTION 0
    #if USE_SUB_SECTION
        sy_subsection_begin(&sy, "sub_section");
        sy_entry(&sy, S_KEY_VALUE(test_f32_s), "%f");
        sy_subsection_end(&sy);
    #endif

    sy_shutdown(&sy);

    LOG(Trace, "test_i32:       [%u]", test_i32)
    LOG(Trace, "test_f32:       [%f]", test_f32)
    LOG(Trace, "test_bool       [%d]", test_bool)
    LOG(Trace, "test_long_long  [%Lf]", test_long_long)

    #if USE_SUB_SECTION
        LOG(Trace, "SubSection: sub_section")
        LOG(Trace, "test_f32_s:         [%f]", test_f32_s)
    #endif

    darray_free(&loop_test_struct_array);
    
#else

    VALIDATE(application_init(argc, argv), logger_shutdown(); return 1, "", "Failed to init the application")
    application_run();
    application_shutdown();

#endif

    crash_handler_shutdown();
    logger_shutdown();
    return 0;
}
