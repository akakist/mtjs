include_directories(/usr/local/include)
add_library(node-serviceS STATIC
    node.cpp
    nodeService.cpp
    sync.cpp
    heart_beat.cpp
    client_node.cpp
    leader_node.cpp
)



