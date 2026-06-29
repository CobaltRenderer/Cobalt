if(COBALT_USE_SDL3)
	add_subdirectory(SDLExample)
endif()

cobalt_install_generated_bin("${COBALT_PROJECT_LICENSE_FILE}" "${COBALT_OUTPUT_ROOT}/Examples")
cobalt_install_generated_bin("${COBALT_THIRD_PARTY_LICENSES_FILE}" "${COBALT_OUTPUT_ROOT}/Examples")
