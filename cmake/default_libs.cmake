include_guard(GLOBAL)

find_package (Threads)

add_library(_zq_DefaultLibs INTERFACE)

target_link_libraries(_zq_DefaultLibs
    INTERFACE Threads::Threads
)

add_library(zq_default::libs ALIAS _zq_DefaultLibs)
