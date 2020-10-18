idf_component_register(
    SRCS "lv-driver/display.cpp" "lv-driver/lv-polyfill.cpp"
    INCLUDE_DIRS "."
    REQUIRES game-link engine-link
)