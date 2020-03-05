#include "http_parsing.hpp"
#include "log.hpp"

namespace SSTT_HTTP_PARSING {

int parse_request_line(Request_Line &rl, char* start) {
    char* sstart = start;
    rl.method = start;
    while (*start != ' ' and *start != '\r' and *start != '\n' and *start != '\0')
        start++;
    if (*start != ' ')
        return -1;
    *start = '\0';

    rl.request_uri = ++start;
    while (*start != ' ' and *start != '\r' and *start != '\n' and *start != '\0')
        start++;
    if (*start != ' ')
        return -1;
    *start = '\0';

    rl.version = ++start;
    while (*start != ' ' and *start != '\r' and *start != '\n' and *start != '\0')
        start++;
    if (*start != '\r' or *(start +1) != '\n')
        return -1;
    *start = '\0';
    start += 2;
    return start-sstart;
}

int parse_header_field(Header_Field &hf, char* start) {
    char* sstart = start;
	hf.field = start;
	while (*start != ':' and *start != '\r' and *start != '\n' and *start != '\0')
    	start++;
	if (*start != ':')
    	return -1;
	*start = '\0';
	start++;
	if (*start != ' ')
    	return -1;
	start++;
	if (*start == ' ')
    	return -1;

	hf.value = start;
	while (*start != '\r' and *start != '\n' and *start != '\0')
    	start++;
    if (*start != '\r' or *(start +1) != '\n')
        return -1;
    *start = '\0';
    start += 2;

	for (char* c = hf.field; *c != 0; c++)
    	*c = tolower(*c);
    
    return start-sstart;
}

} // namespace SSTT_HTTP_PARSING
