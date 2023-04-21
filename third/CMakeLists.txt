include(FetchContent)

# FetchContent_Declare(
#         wheels
#         GIT_REPOSITORY https://gitlab.com/Lipovsky/wheels.git
#         GIT_TAG 74ae096b83365f193cb8d8f75dc047fecff4df01
# )
# FetchContent_MakeAvailable(wheels)

FetchContent_Declare(
        compass
        GIT_REPOSITORY https://gitlab.com/Lipovsky/compass.git
        GIT_TAG 1347897c5a9478c934c35a701b3a53e46198b1a2
)
FetchContent_MakeAvailable(compass)

# FetchContent_Declare(
#         sure
#         GIT_REPOSITORY https://gitlab.com/Lipovsky/sure.git
#         GIT_TAG 7288a84642784e1c31401a3835bdc95e9bbf3de7
# )
# FetchContent_MakeAvailable(sure)

# FetchContent_Declare(
#         tinyfibers
#         GIT_REPOSITORY https://gitlab.com/Lipovsky/tinyfibers.git
#         GIT_TAG e91c30eefa8779c1443ddd8c0f802930dcaf0b92
# )
# FetchContent_MakeAvailable(tinyfibers)


message(STATUS "FetchContent: fmt")

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG 9.1.0
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(tomlplusplus)

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)
