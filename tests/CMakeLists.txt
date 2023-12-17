
add_doctest(test-template
    SOURCES
      template.cpp
)

add_doctest(test-base
    SOURCES
      base/err_test.cpp
      base/context_test.cpp
)

add_doctest(test-commu
    SOURCES
       commu/typedmessage_test.cpp
       commu/hello_test.cpp
       commu/pub_sub_test.cpp
    TIMEOUT 10
)

add_doctest(test-proto
    SOURCES
      proto/base.cpp
)

add_doctest(test-ctx0
    SOURCES
      ctx0/ztx0_test.cpp
)



# non windows
# add_doctest(test-arrayrange
#     SOURCES
#       base/arrayrange_test.cpp
# )
