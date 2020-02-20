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
using namespace std;
using namespace logn;

string message = 
    "GET / HTTP/1.1\r\n"
    "Host: cis.poly.edu\r\n"
    "\r\n";

string answer = 
	"HTTP/1.1 200 OK\r\n"
	"Date: Sun, 26 Sep 2010 20:09:20 GMT\r\n"
	// "Server: Apache/2.0.52 (CentOS)\r\n"
	"Last-Modified: Tue, 30 Oct 2007 17:00:02 GMT\r\n"
	// "ETag: "17dc6-a5c-bf716880"\r\nAccept-Ranges: bytes\r\n"
	"Content-Length: 2652\r\n"
	// "Keep-Alive: timeout=10, max=100\r\n"
	"Connection: Keep-Alive\r\n"
	"Content-Type: text/html; charset=ISO-8859-1\r\n"
	"\r\n"
	"data data data data data ..";




void process_web_request (int fileDescriptor) {
    log << "New petition for fd " << fileDescriptor << endl;
    // Definir buffer y variables necesarias para leer las peticiones
    static const int BUFFER_CAP = 8<<10; // 8Kb
    int bufLen = 0; 
    char buf[BUFFER_CAP+1];

    // Interpret 
    bool header_complete = false;
	while (!header_complete) {
    	int r = read(fileDescriptor, buf+bufLen, BUFFER_CAP-bufLen);
    	if (r < 0) {
        	logerr << "error while reading HTTP header." << endl;
        	continue;
    	}
    	for (int i = 0; i+4 < r; i++) {
        	if (strncmp(buf+bufLen+i, "\r\n\r\n", 4)) {
            	header_complete = true;
        	}
    	}
    	bufLen += r;
	}
	buf[bufLen] = '\0';
	// log << buf << endl;

    //
    // Comprobación de errores de lectura
    //


    //
    // Si la lectura tiene datos válidos terminar el buffer con un \0
    //


    //
    // Se eliminan los caracteres de retorno de carro y nueva linea
    //
	

    //
    //    TRATAR LOS CASOS DE LOS DIFERENTES METODOS QUE SE USAN
    //    (Se soporta solo GET)
	string method = "GET";
	string resource;

    //
    //    Como se trata el caso de acceso ilegal a directorios superiores de la
    //    jerarquia de directorios
    //    del sistema
    


    //
    //    Como se trata el caso excepcional de la URL que no apunta a ningún fichero
    //    html
    //


    //
    //    Evaluar el tipo de fichero que se está solicitando, y actuar en
    //    consecuencia devolviendolo si se soporta u devolviendo el error correspondiente en otro caso


    //
    //    En caso de que el fichero sea soportado, exista, etc. se envia el fichero con la cabecera
    //    correspondiente, y el envio del fichero se hace en blockes de un máximo de  8kB
	string file = "index.html";
    int resource_fd = open(file.c_str(), O_RDONLY);
    if (resource_fd < 0)
        logerr << "Couldn't get access to resource " << 451 << endl << panic(-1);
    int used = 0;
    used += sprintf(buf+used, "HTTP/1.1 200 OK\r\n");
	used += sprintf(buf+used, "Content-Type: text/html; charset=us-ascii\r\n");
    struct stat statbuf;
    if (fstat(resource_fd, &statbuf) < 0) {
        logerr << "fstat fail" << endl;
    }
    used += sprintf(buf+used, "Content-Length: %ld\r\n", statbuf.st_size);
	used += sprintf(buf+used, "\r\n");

	int r;
    do {
        r = read(resource_fd, buf+used, BUFFER_CAP-used);
        if (r < 0) {
            logerr << "error de lectura wey " << endl;
            continue;
        }
        used+=r;
        if (used == BUFFER_CAP || r == 0) {
            int written = 0;
            while (written < used) {
                int w = write(fileDescriptor, buf+written, used-written);
                if (w < 0) {
                	logerr << "error de escritura wey" << endl;
                } else {
                    written += w;
                }
            }
            used = 0;
        }
    } while (r != 0);

    close(fileDescriptor);
    exit(1);
}

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
    if (port < 0 || port >60000)
        cerr << argv[1] << " is not a valid port. Try a port in the range 1-60000"
             << endl << panic();
    if (chdir(argv[2]) == -1)
        cerr << "Couldn't change to directory " << argv[2] << endl << panic();

    // Open log file. Use log and logerr from now on.
    // int fd = open("webserver.log", O_CREAT|O_WRONLY|O_APPEND, 0644);
    // if (fd == -1 || dup2(1, fd) == -1)
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
                process_web_request(socket_fd); // El hijo termina tras llamar a esta función
                exit(0); //TODO comprobar si es buena opción
            default:
                close(socket_fd); //TODO error handling?
        }
    }
}
