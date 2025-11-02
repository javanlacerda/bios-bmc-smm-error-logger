bios_bmc_smm_error_logger_pre = declare_dependency(
    include_directories: [root_inc, bios_bmc_smm_error_logger_inc],
    dependencies: [dependency('threads'), dependency('stdplus')],
)

libfuzzer_args = [
    '-fsanitize=' + get_option('fuzzer-sanitize'),
    '-fno-omit-frame-pointer',
    '-fprofile-instr-generate',
    '-fcoverage-mapping',
    '-g'
]

bios_bmc_smm_error_logger_lib = static_library(
    'bios_bmc_smm_error_logger',
    'pci_handler.cpp',
    'buffer.cpp',
    implicit_include_directories: false,
    dependencies: bios_bmc_smm_error_logger_pre,
    cpp_args: get_option('fuzzing')? libfuzzer_args: [],
)

bios_bmc_smm_error_logger_dep = declare_dependency(
    link_with: bios_bmc_smm_error_logger_lib,
    dependencies: bios_bmc_smm_error_logger_pre,
)

if get_option('fuzzing')
    executable(
        'bios-bmc-smm-error-logger_fuzzer',
        ['fuzzer.cpp', 'read_loop.cpp'],
        conf_h,
        implicit_include_directories: false,
        dependencies: [bios_bmc_smm_error_logger_dep, rde_dep],
        link_args: libfuzzer_args,
        cpp_args: libfuzzer_args,
        install: true,
        install_dir: get_option('bindir'),
    )
else
    executable(
        'bios-bmc-smm-error-logger',
        ['main.cpp', 'read_loop.cpp'],
        conf_h,
        implicit_include_directories: false,
        dependencies: [bios_bmc_smm_error_logger_dep, rde_dep],
        install: true,
        install_dir: get_option('bindir'),
    )
endif
