include(FetchContent)

FetchContent_Declare(
        compass
        GIT_REPOSITORY https://gitlab.com/Lipovsky/compass.git
        GIT_TAG 1347897c5a9478c934c35a701b3a53e46198b1a2
)
FetchContent_MakeAvailable(compass)


# message(STATUS "FetchContent: fmt")

FetchContent_Declare(
        fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG f8c9fabd948e4b3caea30d3c281018b0308491bf
)
FetchContent_MakeAvailable(fmt)

FetchContent_Declare(
        tomlplusplus
        GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
        GIT_TAG v3.3.0
)
FetchContent_MakeAvailable(tomlplusplus)

FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
FetchContent_MakeAvailable(googletest)

FetchContent_Declare(
        function2
        GIT_REPOSITORY https://github.com/Naios/function2.git
        GIT_TAG 2d3a878ef19dd5d2fb188898513610fac0a48621
)
FetchContent_MakeAvailable(function2)
