#ifndef defs_hpp_INCLUDED
#define defs_hpp_INCLUDED

#include<string>
#include<vector>

extern const char* SERVER_NAME;

/** The size of the read/write buffers.
 * Limits the size of the accepted http header
 */
const int BUFFER_CAP  = 8<<10;  // 8Kb
/** The maximum time that the server will wait for a new message */
const int SERVER_TIME_OUT = 5;  // in secons
/** The maximum time that the server will wait between fragments of a message */
const int LATENCY_TIME_OUT = 1; // in seconds

/** Exit code upon error. (for instance, after a failed syscall) */
const int SERVER_ERR  = -1;
/** Exit code upon a failed precondition in the client's message */
const int PRECOND_ERR = -2;

extern int socket_fd;
extern int client_fd;

struct association {
    const char* key;
    const char* value;
};
const std::vector<association> EXTENSIONS {
    {".txt" , "text/plain"},
    {".htm" , "text/html" },
    {".html", "text/html" },
    {".gif" , "image/gif" },
    {".jpg" , "image/jpg" },
    {".jpeg", "image/jpeg"},
    {".png" , "image/png" },
    {".ico" , "image/ico" },
    {".zip" , "image/zip" },
    {".gz"  , "image/gz"  },
    {".tar" , "image/tar" }};

const int STATUS_CONTINUE                 = 100;
const int STATUS_OK                       = 200;
const int STATUS_CREATED                  = 201;
const int STATUS_ACCEPTED                 = 202;
const int STATUS_NO_CONTENT               = 204;
const int STATUS_BAD_REQUEST              = 400;
const int STATUS_UNAUTHORIZED             = 401;
const int STATUS_FORBIDDEN                = 403;
const int STATUS_NOT_FOUND                = 404;
const int STATUS_METHOD_NOT_ALLOWED       = 405;
const int STATUS_REQUEST_TIME_OUT         = 408;
const int STATUS_LENGTH_REQUIRED          = 411;
const int STATUS_PRECONDITION_FAILED      = 412;
const int STATUS_REQUEST_ENTITY_TOO_LARGE = 413;
const int STATUS_REQUEST_URI_TOO_LARGE    = 414;
const int STATUS_UNSUPPORTED_MEDIA_TYPE   = 415;
const int STATUS_INTERNAL_SERVER_ERROR    = 500;
const int STATUS_NOT_IMPLEMENTED          = 501;
const int STATUS_VERSION_NOT_SUPPORTED    = 505;

struct status_code_phrase {
    const int code;
    const char* phrase;
};
const std::vector<status_code_phrase> HTTP_STATUSES {
    {100, "Continue"},
    // {101, "Switching Protocols"},
    {200, "OK"},
    {201, "Created"},
    {202, "Accepted"},
    // {203, "Non-Authoritative Information"},
    {204, "No Content"},
    // {205, "Reset Content"},
    // {206, "Partial Content"},
    // {300, "Multiple Choices"},
    // {301, "Moved Permanently"},
    // {302, "Moved Temporarily"},
    // {303, "See Other"},
    // {304, "Not Modified"},
    // {305, "Use Proxy"},
    {400, "Bad Request"},
    {401, "Unauthorized"},
    // {402, "Payment Required"},
    {403, "Forbidden"},
    {404, "Not Found"},
    {405, "Method Not Allowed"},
    // {406, "Not Acceptable"},
    // {407, "Proxy Authentication Required"},
    {408, "Request Time-out"},
    // {409, "Conflict"},
    // {410, "Gone"},
    {411, "Length Required"},
    {412, "Precondition Failed"},
    {413, "Request Entity Too Large"},
    {414, "Request-URI Too Large"},
    {415, "Unsupported Media Type"},
    {500, "Internal Server Error"},
    {501, "Not Implemented"},
    // {502, "Bad Gateway"},
    // {503, "Service Unavailable"},
    // {504, "Gateway Time-out"},
    {505, "HTTP Version not supported"}
};

/**
 * @brief returns the MIME file format for a given file name.
 * 
 * @param pathname path that identifies the file
 * @return a reference to a string in the MIME format or NULL if the
 * extension is not supported.
 */
const char* content_type(const char* pathname);

/**
 * @brief returns the reason phrase for a given HTTP status code.
 * 
 * This function assumes that the code is supported. That is, it
 * is declared inside HTTP_STATUSES. Otherwise, it returns a
 * random reason phrase.
 */
const char* to_reason_phrase(int code);

#endif // defs_hpp_INCLUDED

