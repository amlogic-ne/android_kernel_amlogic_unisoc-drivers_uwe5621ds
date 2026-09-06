load("//build/kernel/kleaf:kernel.bzl", "kernel_module")

def uwe5621ds_module(name, kernel_build, deps = None):
    kernel_module(
        name = name,
        srcs = ["//vendor/amlogic/unisoc-drivers/uwe5621ds:uwe5621_srcs"],
        makefile = ["//vendor/amlogic/unisoc-drivers/uwe5621ds:Makefile"],
        deps = deps,
        outs = [
            "uwe5621_bsp_sdio.ko",
            "sprdwl_ng.ko",
            "sprdbt_tty.ko",
        ],
        kernel_build = kernel_build,
    )
