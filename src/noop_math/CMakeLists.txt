# noop_math library
add_library (noop_math STATIC)
target_sources(noop_math PRIVATE
        noop_math.h
        noop_math.cpp
)
target_include_directories(noop_math PUBLIC noop_math)