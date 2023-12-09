add_library(ch375mse SHARED ch375mse.cpp)
target_link_options(ch375mse PRIVATE -Wl,-Map=$<TARGET_FILE_DIR:ch375mse>/mapfile.map)
target_link_libraries(ch375mse usbapi ch375 mouse win311headers win311crt c)

create_386(ch375mse CH375MSE)