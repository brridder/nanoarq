cmake_minimum_required(VERSION 3.4)
project(compilation_tests C CXX)

function(add_arq_lib TARGET_NAME TARGET_FLAGS TARGET_SOURCE)
    add_library(${TARGET_NAME} STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME} nanoarq)
    target_compile_options(${TARGET_NAME} PRIVATE
                           -DARQ_ASSERTS_ENABLED=1 -DARQ_USE_C_STDLIB=1 -DARQ_USE_CONNECTIONS=1
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_asserts STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_asserts nanoarq)
    target_compile_options(${TARGET_NAME}_no_asserts PRIVATE
                           -DARQ_ASSERTS_ENABLED=0 -DARQ_USE_C_STDLIB=1 -DARQ_USE_CONNECTIONS=1
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_cstdlib STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_cstdlib nanoarq)
    target_compile_options(${TARGET_NAME}_no_cstdlib PRIVATE
                           -DARQ_ASSERTS_ENABLED=1 -DARQ_USE_C_STDLIB=0 -DARQ_USE_CONNECTIONS=1
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_asserts_no_cstdlib STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_asserts_no_cstdlib nanoarq)
    target_compile_options(${TARGET_NAME}_no_asserts_no_cstdlib PRIVATE
                           -DARQ_ASSERTS_ENABLED=0 -DARQ_USE_C_STDLIB=0 -DARQ_USE_CONNECTIONS=1
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_conn STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_conn nanoarq)
    target_compile_options(${TARGET_NAME}_no_conn PRIVATE
                           -DARQ_ASSERTS_ENABLED=1 -DARQ_USE_C_STDLIB=1 -DARQ_USE_CONNECTIONS=0
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_asserts_no_conn STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_asserts_no_conn nanoarq)
    target_compile_options(${TARGET_NAME}_no_asserts_no_conn PRIVATE
                           -DARQ_ASSERTS_ENABLED=0 -DARQ_USE_C_STDLIB=1 -DARQ_USE_CONNECTIONS=0
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_cstdlib_no_conn STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_cstdlib_no_conn nanoarq)
    target_compile_options(${TARGET_NAME}_no_cstdlib_no_conn PRIVATE
                           -DARQ_ASSERTS_ENABLED=1 -DARQ_USE_C_STDLIB=0 -DARQ_USE_CONNECTIONS=0
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})

    add_library(${TARGET_NAME}_no_asserts_no_cstdlib_no_conn STATIC ${TARGET_SOURCE})
    target_link_libraries(${TARGET_NAME}_no_asserts_no_cstdlib_no_conn nanoarq)
    target_compile_options(${TARGET_NAME}_no_asserts_no_cstdlib_no_conn PRIVATE
                           -DARQ_ASSERTS_ENABLED=0 -DARQ_USE_C_STDLIB=0 -DARQ_USE_CONNECTIONS=0
                           ${ARQ_RUNTIME_FLAGS} ${ARQ_COMMON_FLAGS} ${TARGET_FLAGS})
endfunction()

add_arq_lib(arq_c90 -std=c90 arq_compilation_test.c)
add_arq_lib(arq_c99 -std=c99 arq_compilation_test.c)
add_arq_lib(arq_c11 -std=c11 arq_compilation_test.c)
add_arq_lib(arq_cpp03 -std=c++03 arq_compilation_test.cpp)
add_arq_lib(arq_cpp11 -std=c++11 arq_compilation_test.cpp)
