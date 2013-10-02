/*Non-Canonical Input Processing*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio.h>

#define BAUDRATE B38400
#define _POSIX_SOURCE 1 /* POSIX compliant source */
#define FALSE 0
#define TRUE 1

volatile int STOP=FALSE,RECEBED=FALSE;

void createUAPackage(char * buf){
	buf[0]=0x7e;
	buf[1]=0x03;
	buf[2]=0x07;
	buf[3]=buf[1]^buf[2];
	buf[4]=buf[0];
}

int main(int argc, char** argv)
{
    int fd,c, res;
    int i=0;
    struct termios oldtio,newtio;
    char buf[255];

    if ( (argc < 2) || (strcmp("/dev/ttyS0", argv[1])!=0)) {
      printf("Usage:\tnserial SerialPort\n\tex: nserial /dev/ttyS1\n");
      exit(1);
    }


  /*
    Open serial port device for reading and writing and not as controlling tty
    because we don't want to get killed if linenoise sends CTRL-C.
  */
  
    
    fd = open(argv[1], O_RDWR | O_NOCTTY );
    if (fd <0) {perror(argv[1]); exit(-1); }

    if ( tcgetattr(fd,&oldtio) == -1) { /* save current port settings */
      perror("tcgetattr");
      exit(-1);
    }

    bzero(&newtio, sizeof(newtio));
    newtio.c_cflag = BAUDRATE | CS8 | CLOCAL | CREAD;
    newtio.c_iflag = IGNPAR;
    newtio.c_oflag = 0;

    /* set input mode (non-canonical, no echo,...) */
    newtio.c_lflag = 0;

    newtio.c_cc[VTIME]    = 0;   /* inter-character timer unused */
    newtio.c_cc[VMIN]     = 1;   /* blocking read until 5 chars received */



  /* 
    VTIME e VMIN devem ser alterados de forma a proteger com um temporizador a 
    leitura do(s) pr�ximo(s) caracter(es)
  */



    tcflush(fd, TCIOFLUSH);

    if ( tcsetattr(fd,TCSANOW,&newtio) == -1) {
      perror("tcsetattr");
      exit(-1);
    }

    printf("New termios structure set\n");


    /*while (STOP==FALSE) {       /* loop for input 
      res = read(fd,buf,255);   /* returns after 5 chars have been input 
      buf[res]=0;               /* so we can printf... 
      printf(":%s:%d\n", buf, res);
      if (buf[0]=='z') STOP=TRUE;
    }*/

while(RECEBED==FALSE){
	int j=0;
	while(STOP==FALSE ){
		res=read(fd,&buf[i], 1);
		if(res<0){
			printf("Erro  de leitura");
			exit(1);
		}
		if(buf [i]==0x7e){
			if(j>0){
				STOP=TRUE;
			}
			j++;
		}else{
			i++;
		}

		printf(":%x\n", buf[i]);
	}
	
	//ver se est� completa checa control
	if(buf[3]==0x03)
	{
	createUAPackage(buf);

		printf(":%s\n", buf);
	res = write(fd,buf,5);
	
	RECEBED=TRUE;
	}
	sleep(2);
//}



    tcsetattr(fd,TCSANOW,&oldtio);
    close(fd);

}
    return 0;
}