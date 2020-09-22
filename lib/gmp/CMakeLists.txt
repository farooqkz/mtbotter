add_compile_options(-fPIC)
add_library(gmp mini-gmp.c)
target_link_libraries(gmp)

