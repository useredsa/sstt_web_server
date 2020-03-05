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

/**
 * @brief returns the MIME file format for a given file name..
 * 
 * @param pathname path that identifies the file
 * @return a reference to a string in the MIME format or NULL if the
 * extension is not supported.
 */
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

int send_static_file(int code, const char* pathname, bool keepAlive = true, bool isControlFile = false);

int send_not_found(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_NOT_FOUND);
    return send_static_file(STATUS_NOT_FOUND, "data/not_found.html", true, true);
}
int send_forbidden(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_FORBIDDEN);
    return send_static_file(STATUS_FORBIDDEN, "data/forbidden.html", true, true);
}
int send_internal_server_error(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_INTERNAL_SERVER_ERROR);
    return send_static_file(STATUS_INTERNAL_SERVER_ERROR, "data/internal_server_error.html", true, true);
}
int send_bad_request(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_BAD_REQUEST);
    return send_static_file(STATUS_BAD_REQUEST, "data/bad_request.html", true, true);
}
int send_unauthorized(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_UNAUTHORIZED);
    return send_static_file(STATUS_UNAUTHORIZED, "data/unathorized.html", true, true);
}
int send_unsupported_media_type(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_UNSUPPORTED_MEDIA_TYPE);
    return send_static_file(STATUS_UNSUPPORTED_MEDIA_TYPE, "data/unsupported_media_type.html", true, true);
}
int send_request_entity_too_large(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_REQUEST_ENTITY_TOO_LARGE);
    return send_static_file(STATUS_REQUEST_ENTITY_TOO_LARGE, "data/request_entity_too_large.html", true, true);
}

int send_method_not_allowed(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_METHOD_NOT_ALLOWED);
    return send_static_file(STATUS_METHOD_NOT_ALLOWED, "data/method_not_allowed.html", true, true);
}

int send_version_not_supported(bool useFile = true) {
    if (not useFile)
        return send_status_line(STATUS_VERSION_NOT_SUPPORTED);
    return send_static_file(STATUS_VERSION_NOT_SUPPORTED, "data/version_not_supported.html", true, true);
}

int send_static_file(int code, const char* pathname, bool keepAlive, bool isControlFile) {
    if (access(pathname, F_OK))
        return send_not_found(!isControlFile);
    if (access(pathname, R_OK))
        return send_forbidden(!isControlFile);
    const char* ct = content_type(pathname);
    if (ct == NULL)
        return send_unsupported_media_type(!isControlFile);

    int resource_fd = open(pathname, O_RDONLY);
    if (resource_fd < 0) {
        send_internal_server_error(!isControlFile);
        log << "Couldn't open " << pathname << " !!!" << endl << panic();
        return STATUS_INTERNAL_SERVER_ERROR;
    }
    struct stat statbuf;
    if (fstat(resource_fd, &statbuf) < 0) {
        send_internal_server_error(!isControlFile);
        logerr << "fstat fail" << endl << panic();
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
        bufLen += sprintf(buf+bufLen, "Keep-Alive: timeout=%d\r\n", KA_TIME_OUT);
    }
	bufLen += sprintf(buf+bufLen, "Content-Type: %s\r\n", ct);
    bufLen += sprintf(buf+bufLen, "Content-Length: %ld\r\n", statbuf.st_size);
    
    //TODO resto de cabecera
    //  "Keep-Alive: timeout=10, max=100\r\n"
    // 	"Connection: Keep-Alive\r\n"

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

bool valid_uri(const char* uri) {
    int level = 0;
    const char* last_slash = uri;
    do {
        if (*uri == '\0' or *uri == '/') {
			if (strncmp(last_slash, "..", 2) == 0) {
    			level--;
			} else if (strncmp(last_slash, ".", uri-last_slash) != 0) {
    			level++;
			}
			last_slash = uri+1;
        }
    } while (*uri++ != '\0');
    if (level < 0) return false;
    return true;
}

char* process_get(const Request_Line& rl, char* buf, int& bufLen, char* start, bool& persistent) {
	log << "New GET Request " << rl.method << " " << rl.request_uri << " " << rl.version << endl;

	if (not valid_uri(rl.request_uri)) {
    	send_forbidden();
    	logerr << rl.request_uri << " is not a valid uri" << endl << panic(-2);
	}

	bool containsHost = false;
	// Read header fields
	while (*start != '\r' or *(start+1) != '\n') {
    	Header_Field hf;
    	int r = parse_header_field(hf, start);
    	if (r < 0) {
        	send_bad_request();
        	logerr << "Incorrect header field format" << endl << panic(-2);
    	}
    	if (strcmp(hf.field, "host") == 0) {
        	containsHost = true;
    	} else if (strcmp(hf.field, "connection") == 0) {
        	if (strstr(hf.value, "close") != NULL)
            	persistent = false;
    	}
    	start += r;
    	log << hf.field << ' ' << hf.value << endl;
    	// CHECK HOST
	}
	start+=2;
    bufLen -= start-buf;

    if (not containsHost) {
        send_bad_request();
        log << "The client didn't send a host field!" << endl << panic(-2);
    }

	// Return object
    // Evaluar el tipo de fichero que se está solicitando, y actuar en
    //    consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso
    string pathname = ".";
    pathname += (strcmp(rl.request_uri, "/") != 0? rl.request_uri : "/index.html");
    const char* ct = content_type(pathname.c_str());
    if (ct == NULL) {
        send_unsupported_media_type();
        return start;
    }
    // log << strcmp(rl.request_uri, "/") << endl;
    log << "Returning file " << pathname << endl;
    send_static_file(STATUS_OK, pathname.c_str(), persistent);
    return start;
}

char* process_post(const Request_Line& rl, char* buf, int& bufLen, char* start, bool& persistent) {
    log << "New POST Request " << rl.method << " " << rl.request_uri << " " << rl.version << endl;

	int content_length = 0;
    // Read header fields
	while (*start != '\r' or *(start+1) != '\n') {
    	Header_Field hf;
    	int r = parse_header_field(hf, start);
    	if (r < 0) {
        	send_bad_request();
        	logerr << "Incorrect header field format" << endl << panic(-2);
    	}
    	start += r;

    	if (strcmp(hf.field, "content-length") == 0) {
        	content_length = atoi(hf.value);
    	}
    	//TODO log << hf.field << ' ' << hf.value << endl;
	}
	log << "cl " << content_length << endl;
	start += 2;

	int headSize = start-buf;
	if (content_length+headSize > BUFFER_CAP) {
    	send_request_entity_too_large();
    	log << "The server doesn't support POST messages with more than " << BUFFER_CAP << "bytes" << endl << panic(-2);
	}
	while (bufLen-headSize < content_length) {
		int r = read(client_fd, buf+bufLen, BUFFER_CAP-bufLen);
		if (r < 0) {
    		logerr << "Error while reading HTTP content." << endl;
    		continue;
		}
		bufLen += r;
		if (bufLen-headSize < content_length and r == 0) {
    		send_bad_request();
    		log << "The client didn't complete the body of the message" << endl << panic(-2);
		}
	}
	buf[bufLen] = '\0';
	log << start << endl;
	if (strncmp(start, "email=", 6) == 0 && strcmp(start+6, "emilio.dominguezs%40um.es") == 0) {
    	log << "POST Success!" << endl;
    	send_static_file(STATUS_OK, "success.html", persistent);
	} else {
    	log << "POST Fail!" << endl;
    	send_static_file(STATUS_OK, "failure.html", persistent);
	}
	start += content_length;
    bufLen -= start-buf;
	return start;
}

void process_web_request () {
    int bufLen = 0, r;
    char buf[BUFFER_CAP+1];
    log << "New connection for fd " << client_fd << endl;

    fd_set readFds;
    FD_ZERO(&readFds);
    FD_SET(client_fd, &readFds);
    timeval timeout;
    while (true) {
        timeout.tv_sec = KA_TIME_OUT;
        timeout.tv_usec = 0;
        int sel = select(client_fd+1, &readFds, NULL, NULL, &timeout);
        if (!sel) {
            log << "Timeout!" << endl;
            break;
        }
        // Read HTTP Header (maximum length of BUFFER_CAP)
    	for (bool header_complete = false; !header_complete; buf[bufLen] = '\0') {
        	int r = read(client_fd, buf+bufLen, BUFFER_CAP-bufLen);
        	if (r < 0) {
            	logerr << "Error while reading HTTP header." << endl;
            	//TODO examine error
            	continue;
        	}
        	for (int i = -min(3, bufLen); i+4 <= r; i++)
            	if (strncmp(buf+bufLen+i, "\r\n\r\n", 4) == 0)
                	header_complete = true;
        	bufLen += r;

        	if (!header_complete) {
            	if (bufLen == BUFFER_CAP) {
                	send_request_entity_too_large();
                	log << "The client exceeded the maximum header size" << endl << panic(-2);
            	}
            	if (r == 0) {
                	send_bad_request();
                	log << "The client stopped the connection" << endl << panic(-2);
            	}
        	}
    	}
    	log << "Request:" << buf << endl;

    	//TODO complete message (do inside get or post)

        Request_Line rl;
        r = parse_request_line(rl, buf);

        bool persistent = true;
    	if (strcmp(rl.version, "HTTP/1.0") == 0) {
        	persistent = false;
    	} else if (strcmp(rl.version, "HTTP/1.1") != 0){
        	send_version_not_supported();
        	log << "The client sent an unsuported version" << endl << panic(-1);
    	}

        if (r < 0) {
            send_bad_request();
            close(client_fd); //TODO Shall I close before every panic? Shall I include the close inside panic?
            log << "The client sent an invalid request line" << endl << panic(-2);
        }
        char* consumed;
    	if (strncmp(rl.method, "GET", 3) == 0) {
        	consumed = process_get(rl, buf, bufLen, buf+r, persistent);
    	} else if (strncmp(rl.method, "POST", 4) == 0) {
    		consumed = process_post(rl, buf, bufLen, buf+r, persistent);
    	} else {
        	send_method_not_allowed();
        	log << "Method " << rl.method << " not allowed" << endl << panic(-3);
    	}
    	for (int i = 0; i < bufLen; i++) {
        	buf[i] = consumed[i];
    	}

    	if (not persistent)
        	break;
    }

    close(client_fd);
    log << "Connection finished succesfully" << endl;
}
