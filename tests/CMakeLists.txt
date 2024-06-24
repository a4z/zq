
add_doctest(test-template
    SOURCES
      template.cpp
)

add_doctest(test-base
    SOURCES
      base/err_test.cpp
      base/context_test.cpp
      base/simple_msg_test.cpp
)

add_doctest(test-commu
    SOURCES
       commu/typedmessage_test.cpp
       commu/hello_test.cpp
    TIMEOUT 10
)


add_doctest(test-xtend
    SOURCES
       xtend/rec_v_n_test.cpp
       xtend/empty_message_test.cpp
)

if (ZQ_WITH_PROTO)
    target_sources(test-commu PRIVATE
        commu/typedmessage_proto_test.cpp
        commu/hello_proto_test.cpp
    )
endif()


# keep that extra sice it takes sometiles so long in CI
add_doctest(test-pubsub
    SOURCES
       commu/pub_sub_test.cpp
    TIMEOUT 10
)

if (ZQ_WITH_PROTO)
    add_doctest(test-proto
        SOURCES
        proto/base.cpp
    )
endif()

add_doctest(test-ctx0
    SOURCES
      ctx0/ztx0_test.cpp
)

add_doctest(test-xtra
    SOURCES
       xtra/structs_test.cpp
#        xtra/tuple_test.cpp
       xtra/raise_coverate_test.cpp
# no tuple yet, it's not trivial copiable
)


# non windows
# add_doctest(test-arrayrange
#     SOURCES
#       base/arrayrange_test.cpp
# )
