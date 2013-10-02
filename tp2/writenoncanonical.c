/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <unistd.h>
#define BAUDRATE B38400
#define MODEMDEVICE "/dev/ttyS1"
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP = FALSE, RESEND = FALSE;

int flag = 1, conta = 0;

void sendMessage(char* buf, int* res, int* fd) {
	buf[strlen(buf) - 1] = '\0';
	*res = write(*fd, buf, strlen(buf) + 1);
	printf("%d bytes written\n", *res);
	alarm(3);
	RESEND = FALSE;
}

void createSetPackage(char* buf)
{
	buf[0]=0x7e;
	buf[1]=0x03;
	buf[2]=0x03;
	buf[3]=buf[1] ^ buf[2];
	buf[4]=buf[0];
}

void atende() // atende alarme
{
	printf("alarme # %d\n", conta);
	flag = 1;
	conta++;
	RESEND = TRUE;
}

int main(int argc, char** argv) {
	(void) signal(SIGALRM, atende);
	int fd, res;
	struct termios oldtio, newtio;
	char buf[255];

	if ((argc < 2) || ((strcmp("/dev/ttyS0", argv[1]) != 0))) {
		printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
		exit(1);
	}

	/*
	 Open serial port device for reading and writing and not as controlling tty
	 because we don't want to get killed if linenoise sends CTRL-C.
	 */

	fd = open(argv[1], O_RDWR | O_NOCTTY);
	if (fd < 0) {
		perror(argv[1]);
		exit(-1);
	}

	if (tcgetattr(fd, &oldtio) == -1) { /* save current port settings */
		perror("tcgetattr");
		exit(-1);
	}

	bzero(&newtio, sizeof(newtio));
	newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
	newtio.c_iflag = IGNPAR;
	newtio.c_oflag = 0;

	/* set input mode (non-canonical, no echo,...) */
	newtio.c_lflag = 0;

	newtio.c_cc[VTIME] = 0; /* inter-character timer unused */
	newtio.c_cc[VMIN] = 1; /* blocking read until 5 chars received */

	/* 
	 VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
	 leitura do(s) pr�ximo(s) caracter(es)
	 */

	tcflush(fd, TCIOFLUSH);

	if (tcsetattr(fd, TCSANOW, &newtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	//first try to sendMessage
	printf("New termios structure set\n");
	createSetPackage(buf);
	printf("%s",buf);
	sendMessage(buf, &res, &fd);
	

	int i=0;
	
	
	//timout try
	//while (conta < 3) {
		i = 0;
		while (STOP == FALSE) {
			res = read(fd, &buf[i], 1);
			if (res < 0) {
				printf("Erro  de leitura");
				exit(1);
			}
			if (buf[i] == '\0') {
				STOP = TRUE;
			} else {
				i++;
			}

		}

		if (RESEND == TRUE) {
			sendMessage(buf, &res, &fd);
		}

	}

	/* 
	 O ciclo FOR e as instru��es seguintes devem ser alterados de modo a respeitar 
	 o indicado no gui�o 
	 */

	if (tcsetattr(fd, TCSANOW, &oldtio) == -1) {
		perror("tcsetattr");
		exit(-1);
	}

	close(fd);
	return 0;
}

