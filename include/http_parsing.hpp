#ifndef http_parsing_hpp_INCLUDED
#define http_parsing_hpp_INCLUDED

namespace SSTT_HTTP_PARSING {




struct Request_Line {
    char* method;
    char* request_uri;
    char* version;
};

struct Header_Field {
    char* field;
    char* value;
};

int parse_request_line(Request_Line& rl, char* start);
int parse_header_field(Header_Field& hf, char* start);

/**
 * @brief Current implementation only checks that the uri
 * doesn't reference content above the parent directory.
 */
bool valid_uri(const char* uri);

}

#endif // http_parsing_hpp_INCLUDED

