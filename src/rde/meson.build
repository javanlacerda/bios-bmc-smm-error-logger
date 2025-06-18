rde_pre = declare_dependency(
    include_directories: [rde_inc],
    dependencies: [
        dependency('libbej'),
        dependency('nlohmann_json', include_type: 'system'),
        dependency('phosphor-dbus-interfaces'),
        dependency('sdbusplus'),
        dependency('stdplus'),
    ],
)

libfuzzer_args = [
    #'-fsanitize=fuzzer,address',  # Add 'undefined' or 'memory' if needed
    '-fsanitize=fuzzer',  # Add 'undefined' or 'memory' if needed
    '-fno-omit-frame-pointer',
    '-fprofile-instr-generate', 
    '-fcoverage-mapping',
    '-g'
]

rde_lib = static_library(
    'rde',
    'rde_dictionary_manager.cpp',
    'external_storer_file.cpp',
    'rde_handler.cpp',
    'notifier_dbus_handler.cpp',
    implicit_include_directories: false,
    dependencies: rde_pre,
    cpp_args: libfuzzer_args,
)

rde_dep = declare_dependency(link_with: rde_lib, dependencies: rde_pre)
