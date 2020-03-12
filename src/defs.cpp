#include "defs.hpp"

#include <string.h>

const char* SERVER_NAME = "ED S.A. - Servicios Telemáticos";

int socket_fd;
int client_fd = 0;

const char* content_type(const char* pathname) {
	const char* ext = strrchr(pathname, '.');
	if (ext != NULL) {
    	for (const auto& ass : EXTENSIONS) {
        	if (strcmp(ass.key, ext) == 0) {
            	return ass.value;
        	}
    	}
	}
	return NULL;
}

const char* to_reason_phrase(int code) {
	for (const auto& ass : HTTP_STATUSES) {
    	if (ass.code == code) {
        	return ass.phrase;
    	}
	}
	// Prefer to return a random value than to crash the server with a NULL value.
	return HTTP_STATUSES[0].phrase;
}

