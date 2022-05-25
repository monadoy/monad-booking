patch:
	@echo -e "\nPatching ESPAsyncWebserver library. See https://github.com/me-no-dev/ESPAsyncWebServer/issues/1085\n"

	sed -i 's/mbedtls_md5_starts;/mbedtls_md5_init/' .pio/libdeps/m5stack-fire/ESP\ Async\ WebServer/src/WebAuthentication.cpp
	sed -i 's/mbedtls_md5_update/mbedtls_md5_update_ret/' .pio/libdeps/m5stack-fire/ESP\ Async\ WebServer/src/WebAuthentication.cpp
	sed -i 's/mbedtls_md5_finish.*/mbedtls_md5_finish(\&_ctx, _buf);\n  mbedtls_internal_md5_process( \&_ctx ,data);/' .pio/libdeps/m5stack-fire/ESP\ Async\ WebServer/src/WebAuthentication.cpp

	@echo -e "\nLibrary patched! You may now build normally"

littlefs:
	./fetchmklittlefs.sh
	
prebuild: littlefs patch