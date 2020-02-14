#ifndef defs_hpp_INCLUDED
#define defs_hpp_INCLUDED

#include<string>
#include<vector>

const int VERSION    =  24;
const int ERROR      =  42;
const int LOG        =  44;
const int PROHIBITED = 403;
const int NOTFOUND   = 404;

struct association {
    std::string ext;
    std::string filetype;
};
const std::vector<association> extensions {
    {"gif" , "image/gif" },
    {"jpg" , "image/jpg" },
    {"jpeg", "image/jpeg"},
    {"png" , "image/png" },
    {"ico" , "image/ico" },
    {"zip" , "image/zip" },
    {"gz"  , "image/gz"  },
    {"tar" , "image/tar" },
    {"htm" , "text/html" },
    {"html", "text/html" }};

extern int socket_fd;

#endif // defs_hpp_INCLUDED

