#include "message_processing.hpp"

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include "defs.hpp"
#include "http_parsing.hpp"
#include "log.hpp"
using namespace logn;
using namespace SSTT_HTTP_PARSING;
using namespace std;

int send_static_file(int code, const char* pathname,
                     bool keepAlive = true, bool isInternalFile = false);

int send_not_found(bool keepAlive = true) {
    return send_static_file(STATUS_NOT_FOUND, "data/not_found.html",
                            keepAlive, true);
}

int send_forbidden(bool keepAlive = true) {
    return send_static_file(STATUS_FORBIDDEN, "data/forbidden.html",
                            keepAlive, true);
}

int send_internal_server_error(bool keepAlive = false) {
    return send_static_file(STATUS_INTERNAL_SERVER_ERROR, "data/internal_server_error.html",
                            keepAlive, true);
}

int send_bad_request(bool keepAlive = false) {
    return send_static_file(STATUS_BAD_REQUEST, "data/bad_request.html",
                            keepAlive, true);
}

int send_unauthorized(bool keepAlive = true) {
    return send_static_file(STATUS_UNAUTHORIZED, "data/unathorized.html",
                            keepAlive, true);
}

int send_unsupported_media_type(bool keepAlive = true) {
    return send_static_file(STATUS_UNSUPPORTED_MEDIA_TYPE, "data/unsupported_media_type.html",
                            keepAlive, true);
}

int send_request_entity_too_large(bool keepAlive = false) {
    return send_static_file(STATUS_REQUEST_ENTITY_TOO_LARGE,
                            "data/request_entity_too_large.html",
                            keepAlive, true);
}

int send_method_not_allowed(bool keepAlive = false) {
    return send_static_file(STATUS_METHOD_NOT_ALLOWED, "data/method_not_allowed.html",
                            keepAlive, true);
}

int send_version_not_supported(bool keepAlive = false) {
    return send_static_file(STATUS_VERSION_NOT_SUPPORTED, "data/version_not_supported.html",
                            keepAlive, true);
}

int send_not_implemented(bool keepAlive = false) {
    return send_static_file(STATUS_NOT_IMPLEMENTED, "data/version_not_supported.html",
                            keepAlive, true);
}

int send_status_line(int code) {
    char buf[BUFFER_CAP];
    int bufLen = sprintf(buf, "%s %d %s\r\n\r\n", HTTP_VERSION, code, to_reason_phrase(code));
    char* off = buf;
    while (bufLen > 0) {
        int w = write(client_fd, off, bufLen);
        if (w < 0) {
            logerr << "error de escritura in my face!" << endl;
            continue;
        }
        bufLen-=w;
        off+=w;
    }
    return code;
}

int send_static_file(int code, const char* pathname, bool keepAlive, bool isInternalFile) {
    const char* ct = content_type(pathname);
    if (ct == NULL) {
        if (isInternalFile) return send_status_line(code);
        else return send_unsupported_media_type();
    }
    if (access(pathname, F_OK)) { //TODO enviar el mismo código solo con status_line
        if (isInternalFile) return send_status_line(code);
        else return send_not_found();
    }
    if (access(pathname, R_OK)) {
        if (isInternalFile) return send_status_line(code);
        else return send_forbidden();
    }

    int resource_fd = open(pathname, O_RDONLY);
    if (resource_fd < 0) { //TODO es internal server error?
        if (isInternalFile) return send_status_line(code);
        else {
            send_internal_server_error();
            logerr << "fstat fail" << endl << panic();
        }
    }
    struct stat statbuf;
    if (fstat(resource_fd, &statbuf) < 0) {
        if (isInternalFile) return send_status_line(code);
        else {
            send_internal_server_error();
            logerr << "fstat fail" << endl << panic();
        }
    }

    char buf[2*BUFFER_CAP];
    int bufLen = sprintf(buf, "%s %d %s\r\n", HTTP_VERSION, code, to_reason_phrase(code));
	bufLen += sprintf(buf+bufLen, "Server: %s\r\n", SERVER_NAME);
    time_t raw_time;
    char* time_string;
    if (time(&raw_time) < 0 || (time_string = ctime(&raw_time)) == NULL) {
        logerr << "error getting time!" << endl;
    } else {
        bufLen += sprintf(buf+bufLen, "Date: %s", time_string);
        buf[bufLen-1] = '\r';
        buf[bufLen] = '\n';
        bufLen++;
    }
    bufLen += sprintf(buf+bufLen, "Connection: %s\r\n", keepAlive? "keep-alive" : "close");
    if (keepAlive) {
        bufLen += sprintf(buf+bufLen, "Keep-Alive: timeout=%d\r\n", SERVER_TIME_OUT);
    }
	bufLen += sprintf(buf+bufLen, "Content-Type: %s\r\n", ct);
    bufLen += sprintf(buf+bufLen, "Content-Length: %ld\r\n", statbuf.st_size);
    
	bufLen += sprintf(buf+bufLen, "\r\n");

    int r;
    char* off = buf;
    do {
        r = read(resource_fd, off+bufLen, BUFFER_CAP);
        if (r < 0) {
            logerr << "error de lectura wey " << endl << panic(-1);
        }
        bufLen += r;
        // Write in blocks of BUFFER_CAP (except possibly the last one)
        while (bufLen > BUFFER_CAP || (bufLen > 0 && r == 0)) {
            int w = write(client_fd, off, min(BUFFER_CAP, bufLen));
            if (w < 0) {
            	logerr << "error de escritura wey" << endl;
            }
            off += w;
            bufLen -= w;
        }

        if (off + bufLen > buf+BUFFER_CAP) {
            for (int i = 0; i < bufLen; i++) {
                buf[i] = off[i];
            }
            off = buf;
        }
    } while (r != 0);
    return code;
}

/**
 * @brief Processes a GET request
 * 
 * This function may read extra data into the read buffer to complete
 * its message. Therefore, any reference to a fragment of the message
 * (like the request line) MUST be considered invalidated.
 * The function may update the persistent behaviour according the the
 * header connection field.
 */
char* process_get(const Request_Line& rl, char* buf, int& bufLen,
                  char* bufoff, bool& persistent) {
	log << "New GET Request for " << rl.request_uri << " " << rl.version << endl;

	if (!valid_uri(rl.request_uri)) {
    	send_forbidden();
    	logerr << rl.request_uri << " is not a valid uri" << endl << panic(PRECOND_ERR);
	}

	bool contains_host = false;
	int content_length = 0;
	// Read header fields
	while (bufoff[0] != '\r' || bufoff[1] != '\n') {
    	Header_Field hf;
    	int hf_size = parse_header_field(hf, bufoff);
    	if (hf_size < 0) {
        	send_bad_request();
        	logerr << "Incorrect header field format" << endl << panic(PRECOND_ERR);
    	}
    	if (strcmp(hf.field, "host") == 0) {
        	contains_host = true;
    	} else if (strcmp(hf.field, "connection") == 0) {
        	if (strstr(hf.value, "close") != NULL) {
            	persistent = false;
        	}
    	} else if (strcmp(hf.field, "content-length") == 0) {
        	content_length = strtol(hf.value, NULL, 10);
    	}
    	bufoff += hf_size;
    	// log << hf.field << ' ' << hf.value << endl;
    	// CHECK HOST
	}
	bufoff += 2; // Discard last "\r\n".
    bufLen -= bufoff-buf;

    if (!contains_host) {
        send_bad_request();
        log << "The client didn't send a host field!" << endl << panic(PRECOND_ERR);
    }

    if (content_length != 0) {
        send_not_implemented();
        log << "The client sent content inside GET, which isn't supported."
            << endl << panic(PRECOND_ERR);
    }

	/**
     * Now that we've interpreted the complete header, we must
     * evaluate the type of file that the client is asking for
     * and return the file if it's supported or the appropiate
     * error.
     */

    // Make the route relative.
    char* rel_uri = rl.request_uri;
    while (*rel_uri == '/')
        rel_uri++;
    const char* pathname = *rel_uri == '\0'? "index.html" : rel_uri;

    log << "GET process complete. Send: " << pathname << endl;
    send_static_file(STATUS_OK, pathname, persistent);
    return bufoff;
}

/**
 * @brief Processes a POST request
 * 
 * This function may read extra data into the read buffer to complete
 * its message. Therefore, any reference to a fragment of the message
 * (like the request line) MUST be considered invalidated.
 * The function may update the persistent behaviour according the the
 * header connection field.
 */
char* process_post(const Request_Line& rl, char* buf, int& bufLen,
                   char* bufoff, bool& persistent) {
    log << "New POST Request for " << rl.request_uri << " " << rl.version << endl;

	bool contains_host = false;
	int content_length = 0;
    // Read header fields
	while (*bufoff != '\r' or *(bufoff+1) != '\n') {
    	Header_Field hf;
    	int r = parse_header_field(hf, bufoff);
    	if (r < 0) {
        	send_bad_request();
        	logerr << "Incorrect header field format" << endl << panic(-2);
    	}
    	bufoff += r;

        if (strcmp(hf.field, "host") == 0) {
            contains_host = true;
        } else if (strcmp(hf.field, "content-length") == 0) {
        	content_length = strtol(hf.value, NULL, 10);
    	}
    	//TODO log << hf.field << ' ' << hf.value << endl;
	}
	log << "content-length: " << content_length << endl;
	bufoff += 2;

	int headSize = bufoff-buf;
	if (!contains_host) {
    	send_bad_request();
    	log << "Post request didn't contain host field" << endl << panic(PRECOND_ERR);
	}
	if (content_length < 0) {
    	send_bad_request();
    	log << panic(PRECOND_ERR);
	}
	if (content_length+headSize > BUFFER_CAP) {
    	send_request_entity_too_large();
    	log << "The server doesn't support POST messages with more than "
    	    << BUFFER_CAP << "bytes" << endl << panic(PRECOND_ERR);
	}
	while (bufLen-headSize < content_length) {
		int r = read(client_fd, buf+bufLen, BUFFER_CAP-bufLen);
		if (r < 0) {
    		//TODO send_internal_server_error();
    		logerr << "Error while reading HTTP content." << endl << panic(SERVER_ERR);
		}
		bufLen += r;
		if (r == 0 && bufLen-headSize < content_length) {
    		send_bad_request();
    		log << "The client didn't complete the body of the message"
    		    << endl << panic(PRECOND_ERR);
		}
	}
	buf[bufLen] = '\0';

	//TODO preguntar qué hacer con un post que contiene una uri inválida
	log << "Post content: " << bufoff << endl;
	if (strcmp(bufoff, "email=emilio.dominguezs%40um.es") == 0) {
    	log << "POST Success!" << endl;
    	send_static_file(STATUS_OK, "success.html", persistent);
	} else {
    	log << "POST Fail!" << endl;
    	send_static_file(STATUS_OK, "failure.html", persistent);
	}
	bufoff += content_length;
    bufLen -= bufoff-buf;
	return bufoff;
}

/**
 * @brief Anwers all the petitions of a client until connection is closed.
 */
void deal_with_client() {
    log << "New client connection" << endl;
    // Reading buffer
    int bufLen = 0;
    char buf[BUFFER_CAP+1];
    // Timeout structures
    fd_set readFds;
    timeval timeout;

    // Process upcoming HTTP messages
    while (true) {
        // Wait (up to SERVER_TIME_OUT) for data to read 
        FD_ZERO(&readFds);
        FD_SET(client_fd, &readFds);
        timeout.tv_sec = SERVER_TIME_OUT;
        timeout.tv_usec = 0;
        if (!select(client_fd+1, &readFds, NULL, NULL, &timeout)) {
            //TODO send_request_time_out();
            log << "SERVER_TIME_OUT" << endl << panic(0);
        }
        // Read HTTP Header (maximum length of BUFFER_CAP)
    	for (bool header_complete = false; !header_complete; buf[bufLen] = '\0') {
        	// Perform read operation
        	int r = read(client_fd, buf+bufLen, BUFFER_CAP-bufLen);
        	if (r < 0) {
            	//TODO send_internal_server_error();
            	logerr << "Error while reading HTTP header." << endl << panic(SERVER_ERR);
            	//TODO We could examine the errors
        	}

        	// Check for the end of the header ("\r\n\r\n")
        	if (strstr(buf+max(bufLen-3, 0), "\r\n\r\n") != NULL) {
            	header_complete = true;
        	}
        	bufLen += r;

            // Error Control
        	if (!header_complete) {
            	if (bufLen == BUFFER_CAP) {
                	send_request_entity_too_large();
                	log << "The client filled the buffer with the header"
                	    << endl << panic(PRECOND_ERR);
            	}
            	if (r == 0) {
                	if (bufLen > 0) {
                    	send_bad_request();
                    	log << buf << endl;
                    	log << "The client stopped the connection in the middle "
                    	    << "of the header" << endl << panic(PRECOND_ERR);
                	} else {
                    	log << "No new message. The connection stopped "
                    	    << "succesfully." << endl << panic(0);
                	}
            	}
                FD_ZERO(&readFds);
                FD_SET(client_fd, &readFds);
                timeout.tv_sec = LATENCY_TIME_OUT;
                timeout.tv_usec = 0;
                if (!select(client_fd+1, &readFds, NULL, NULL, &timeout)) {
                    //TODO send_request_time_out();
                    log << "LATENCY_TIME_OUT" << endl << panic(PRECOND_ERR);
                    break;
                }
        	}
    	}
    	// log << "Request:" << buf << endl;
    	/* Upon reaching this point we have the HTTP request header. We may
         * have read part of the content or even part of the next message
         * if the client is performing pipelining. Method functions shall
         * return how many bytes they consumed from the message.
         * 
         * We will store the start of the new data in the variable consumed.
         */

        Request_Line rl;
        int rl_size = parse_request_line(rl, buf);
        if (rl_size < 0) {
            send_bad_request();
            log << "The client sent an invalid request line" << endl << panic(PRECOND_ERR);
        }

        bool persistent = true;
    	if (strcmp(rl.version, "HTTP/1.0") == 0) {
        	persistent = false;
    	} else if (strcmp(rl.version, "HTTP/1.1") != 0){
        	send_version_not_supported();
        	log << "The client sent an unsuported version" << endl << panic(PRECOND_ERR);
    	}

        char* consumed;
    	if (strcmp(rl.method, "GET") == 0) {
        	consumed = process_get(rl, buf, bufLen, buf+rl_size, persistent);
    	} else if (strcmp(rl.method, "POST") == 0) {
    		consumed = process_post(rl, buf, bufLen, buf+rl_size, persistent);
    	} else {
        	send_not_implemented();
        	log << "Method " << rl.method << " not allowed" << endl << panic(PRECOND_ERR);
    	}

    	/* Move the excessive data to the beginning of the buffer */
    	for (int i = 0; i < bufLen; i++) {
        	buf[i] = consumed[i];
    	}

    	if (not persistent) {
            log << "No persistency. The connection finished succesfully." << endl << panic(0);
    	}
    }
}
