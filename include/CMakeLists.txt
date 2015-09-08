## configure pkgconfig files ##
CONFIGURE_FILE(tts.pc.in "${PROJECT_NAME}.pc" @ONLY)
CONFIGURE_FILE(tts-setting.pc.in "${PROJECT_NAME}-setting.pc" @ONLY)
CONFIGURE_FILE(tts-engine.pc.in "${PROJECT_NAME}-engine.pc" @ONLY)

## Install header, pc files ##
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}.pc" DESTINATION lib/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}-setting.pc" DESTINATION lib/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/${PROJECT_NAME}-engine.pc" DESTINATION lib/pkgconfig)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/tts.h" DESTINATION include)
INSTALL(FILES "${CMAKE_BINARY_DIR}/include/tts_setting.h" DESTINATION include)
INSTALL(FILES "${CMAKE_BINARY_DIR}/server/ttsp.h" DESTINATION include)
