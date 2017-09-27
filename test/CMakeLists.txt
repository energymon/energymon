# Tests

add_executable(interface-test interface_test.c)
target_link_libraries(interface-test energymon-default)

add_executable(interval-test interval_test.c)
target_link_libraries(interval-test energymon-default)
