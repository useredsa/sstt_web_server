#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "defs.hpp"
#include "log.hpp"
#include "message_processing.hpp"
using namespace std;
using namespace logn;




/**
 * @brief //TODO
 * //TODO
 * @param argv[1] Port for the server socket.
 * @param argv[2] Folder with the server contents.
 */
int main(int argc, char **argv) {
    static struct sockaddr_in cli_addr;     // static => default initialization (with 0s)
    static struct sockaddr_in serv_addr;    // static => default initialization (with 0s)
    int port, listenfd;

    // Parameter Verification
    if (argc != 3)
        cerr << "Usage: " << argv[0] << " port folder" << endl << panic();
    port = atoi(argv[1]);
    if (port < 0 or port >60000)
        cerr << argv[1] << " is not a valid port. Try a port in the range 1-60000"
             << endl << panic();
    if (chdir(argv[2]) == -1)
        cerr << "Couldn't change to directory " << argv[2] << endl << panic();

    // Open log file. Use log and logerr from now on.
    // int fd = open("webserver.log", O_CREAT|O_WRONLY|O_APPEND, 0644);
    // if (fd == -1 or dup2(1, fd) == -1)
    //     cerr << "Coudln't create log file. Logs will be written to terminal" << endl;

    // Behave as a daemon.
    switch (fork()) {
        case -1:
            logerr << "fork() fail" << endl << panic();
        case 0:
            signal(SIGCHLD, SIG_IGN);  // Ignore children
            signal(SIGHUP,  SIG_IGN);  // Ignore Hang up from terminal
            log << "web server starting on port " << port <<  "...\n";
            break;
        default:
            return 0;             // Return control to the user instantly. 
    }

    // Set up the network socket
    if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        logerr << "Coudln't set up the network socket" << endl << panic();


    // Create an structure for the socket (IP address and port) where the server listens.
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);  // Listen to any possible IP
    serv_addr.sin_port = htons(port);               // on port `port`.

    if (bind(listenfd, (struct sockaddr*) &serv_addr, sizeof(serv_addr)) < 0)
        logerr << "bind fail" << endl << panic();
    if (listen(listenfd, 64) < 0)
        logerr << "listen fail" << endl << panic();

    while (true) {
        socklen_t length = sizeof(cli_addr);
        if ((socket_fd = accept(listenfd, (struct sockaddr *) &cli_addr, &length)) < 0)
            logerr << "accept fail" << endl << panic();
        switch (fork()) {
            case -1:
                logerr << "fork() fail" << endl << panic();
            case  0:
                close(listenfd);
                client_fd = socket_fd;
                deal_with_client(); // El hijo termina tras llamar a esta función
                log << "Child finished." << endl << panic(0); //TODO comprobar si es buena opción
            default:
                close(socket_fd); //TODO error handling?
        }
    }
}
