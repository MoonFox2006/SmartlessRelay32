idf_component_register(
    SRCS "mqtt.cpp" "percent.c" "TeleBot.cpp" "loopdns.cpp" "relays.cpp" "logger.cpp" "global.cpp" "main.cpp" "buttons.c" "blinks.c" "config.cpp" "CharBuf.cpp" "HttpResp.cpp" "ntp.cpp" "web.cpp"
    INCLUDE_DIRS "."
    EMBED_FILES "favicon.ico"
)

set_property(TARGET ${COMPONENT_LIB} PROPERTY CXX_STANDARD 17)
#target_compile_options(${COMPONENT_LIB} PUBLIC "-DUSE_ESP_IDF_LOG")
#target_compile_options(${COMPONENT_LIB} PRIVATE "-DCHIP_HAVE_CONFIG_H")
