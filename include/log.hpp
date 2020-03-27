/**
 * @brief (Super) Minimalistic Logging functionality for the project.
 * @author Emilio Domínguez Sánchez
 * 
 * For extensive comprehension is better to read the code.
 * Nevertheless, we provide some examples.
 * Usage:
 *        Code: log << "alberto, tenemos un problema de tipo " << type << endl;
 *        Output: INFO(socket ---): alberto, tenemos un problema de tipo ---
 *        Code: log << endl;
 *        Output:
 *        Code: log << "mensaje de log 1" << endl;
 *              log << "mensaje de log 2\n";
 *        Output: INFO(socket ---): mensaje de log 1
 *                INFO(socket ---): mensaje de log 2
 *        Code: logerr << "falló una llamada al sistema" << endl;
 *        Output: ERROR: errno=--- exiting pid=---: falló una llamada al sistema
 *        Code: logerr << "problemas Mike!" << endl << panic();
 *        Output: ERROR: errno=--- exiting pid=---: problemas Mike!
 *                -> Program finishes execution with code -1
 */
#ifndef log_hpp_INCLUDED
#define log_hpp_INCLUDED

#include <iostream>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

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
        os << "INFO(socket " << client_fd << "): ";
    }
} log(cout);

static class ErrorLogger : public Logger {
public:
    ErrorLogger (ostream& os) : Logger(os) {}

private:
    void outputSignature(){
        os << "ERROR: errno=" << errno << " strerror=" << strerror(errno) << " exiting pid=" << getpid() << ": ";
    }
} logerr(cout);


class panic {
public:
    panic(int exitCode = -1) : exitCode(exitCode) {}
    int exitCode = -1;
};

static inline void operator<< (const ostream& log, const panic& p) {
    close(client_fd); //TODO consider
    exit(p.exitCode);
}

} // namespace log


#endif // log_hpp_INCLUDED

