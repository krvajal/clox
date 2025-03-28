# BUILD file for the clox project

cc_library(
    name = "clox_lib",
    srcs = glob(["src/**/*.c"]),
    hdrs = glob(["src/**/*.h"]),
    includes = ["src"],
    visibility = ["//visibility:public"],
)

cc_binary(
    name = "clox",
    srcs = ["src/main.c"],
    deps = [":clox_lib"],
)