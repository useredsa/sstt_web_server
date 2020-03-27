#ifndef http_parsing_hpp_INCLUDED
#define http_parsing_hpp_INCLUDED

namespace SSTT_HTTP_PARSING {

/**
 * @brief Request Line reference structure
 */
struct Request_Line {
    char* method;
    char* request_uri;
    char* version;
};

/**
 * @brief Header Field reference structure
 */
struct Header_Field {
    char* field;
    char* value;
};

/**
 * @brief Parses the first line of start as a Request Line.
 * 
 * Modifies the contents of start making each token end in
 * a null character instead of a delimiter.
 */
int parse_request_line(Request_Line& rl, char* start);
/**
 * @brief Parses the first line of start as a Header Field.
 * 
 * Modifies the contents of start making each token end in
 * a null character instead of a delimiter.
 */
int parse_header_field(Header_Field& hf, char* start);

/**
 * @brief Uniform Resource Identifier Validity check.
 * 
 * Current implementation only checks that the uri
 * doesn't reference content above the parent directory.
 */
bool valid_uri(const char* uri);

}

#endif // http_parsing_hpp_INCLUDED

