#ifndef log_hpp_INCLUDED
#define log_hpp_INCLUDED

#include <iostream>

#include "defs.hpp"

namespace logn {

using std::ostream;
using std::cout;

class IinlineLogger {
public:
    IinlineLogger (ostream& os) : os(os) {}

    template<typename T>
    IinlineLogger& operator<< (T output) {
       os << output;
       return *this;
    }

	// A bit of advanced cpp code for supporting endl
    IinlineLogger& operator<< (ostream& (*f)(ostream&)) {
        f(os);
        return *this;
    }

protected:
    ostream& os;
};

class Logger : public IinlineLogger {
public:
    template<typename T>
    IinlineLogger& operator<< (T output) {
        outputSignature();
        os << output;
        return *this;
    }

    Logger(ostream& os) : IinlineLogger(os) {}

protected:
	virtual void outputSignature() = 0;
};




static class InfoLogger : public Logger {
public:
    InfoLogger (ostream& os) : Logger(os) {}

private:
    void outputSignature(){
        os << "INFO(socket " << socket_fd << "): ";
    }
} log(cout);

static class ErrorLogger : public Logger {
public:
    ErrorLogger (ostream& os) : Logger(os) {}

private:
    void outputSignature(){
        os << "ERROR: Errno=" << errno << " exiting pid=" << getpid() << ": ";
    }
} logerr(cout);


class panic {
public:
    panic(int exitCode = -1) : exitCode(exitCode) {}
    int exitCode = -1;
};

void operator<< (const ostream& os, const panic& p) {
    exit(p.exitCode);
}

} // namespace log


#endif // log_hpp_INCLUDED

