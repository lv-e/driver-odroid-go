idf_component_register(
    SRCS "main"
    INCLUDE_DIRS "../components"
    REQUIRES game-link driver-link engine-link
)