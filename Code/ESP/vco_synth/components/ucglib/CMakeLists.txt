# ucglib (oude C-versie) als los ESP-IDF-component.
# Alle bestanden worden gecompileerd; fonts die je niet gebruikt gooit de linker er weer uit.
file(GLOB UCG_SRCS "csrc/*.c")
idf_component_register(SRCS ${UCG_SRCS}
                       INCLUDE_DIRS "csrc")
# Oude bibliotheek: waarschuwingen uit, anders breekt ESP-IDF (-Werror=all) de build af
target_compile_options(${COMPONENT_LIB} PRIVATE -w)
