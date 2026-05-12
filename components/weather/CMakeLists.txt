idf_component_register(
    SRCS weather.c
    INCLUDE_DIRS .
    REQUIRES wifi esp_http_client cjson esp-tls
)