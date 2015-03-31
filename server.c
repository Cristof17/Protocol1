#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include "lib.h"

#define HOST "127.0.0.1"
#define PORT 10001
#define CD 11
#define LS 12
#define CP 13
#define SN 14
#define EXIT 15

#define EVEN 0
#define UNEVEN 1

#define PARITY 112
#define NOPARITY 1000

#define ACK 100
#define NACK 101


 int equalsCommand(char * payload);
 int getNumberOfFiles(DIR * dir);
 char * getSecondArgument(char * commandRequest);
 int sendACK(msg *t);
 void sendNumberOfFiles(msg *t, char* from);
 void receiveACK(msg *r);
 void sendFileNames(char* path, msg *t , msg *r);
 void cd(char * path);
 void sendFile(char *name, msg *t, msg *r);
 void sendFileSize(char * path , msg *t);
 void createFile(char * path , char * returnValue);
 int receiveFileSize(msg * r);
 void writeInFile(char * path ,int nrBytes , msg * r , msg *t );
 char checkParity(msg * r);
 char getParity(char * payload ) ;
 void sendNACK(msg *t);
 int whatTestIsIt(char ** argv);
 int receiveResponse( msg * r);
 void processMessageReceive(char * outputCommand , msg * t ,msg *r );

 int TYPE = 0;

int main(int argc , char ** argv)
{
	msg r ,t;
	int i, res;

	printf("[SERVER] Starting.\n");
	init(HOST, PORT);

  if(argv[1] == NULL){
     TYPE = NOPARITY;
     printf("[SERVER] Parity is NOPARITY \n\r");
  }
  else if(strcmp (argv[1] , "parity") == 0){
     TYPE = PARITY ;
     printf("[SERVER] Parity is PARITY \n\r");
  }

  char command[MSGSIZE];

	for (i = 0; i < COUNT; i++) {
		/* wait for message */

		res = recv_message(&r);
		if (res < 0) {
			perror("[SERVER] Receive error. Exiting.\n");
			return -1;
		}else{
      printf("[SERVER] Received message with data:%s\n\r" , r.payload);

     while (checkParity(&r) != getParity(r.payload)){
        printf("[SERVER] Corruption occured \n\r");
        printf("[SERVER] Parity of the message is %d\n\r",checkParity(&r));
        printf("[SERVER] Calculated Parity is %d\n\r",getParity(r.payload));
        sendNACK(&t);
        recv_message(&r);
     }
        printf("[SERVER] No Corruption occured \n\r");
        printf("[SERVER] Parity of the message is %d\n\r",checkParity(&r));
        printf("[SERVER] Calculated Parity is %d\n\r",getParity(r.payload));

      if(TYPE == PARITY){
          printf("[SERVER] Core message is %s \n\r",r.payload);
          memcpy(command, r.payload+1 , r.len - 1 );
          printf("[SERVER] Coredata is %s\n\r",command );
      }else
          memcpy(command , r.payload , r.len );

     //process the file
     if(equalsCommand(command) == LS){


       printf("[SERVER] Command is %s\n\r",command);
       printf("[SERVER] Second argument is %s\n\r",getSecondArgument(command));

       sendACK(&t);
       sendNumberOfFiles(&t,getSecondArgument(command));
       receiveACK(&r);
//       printf("[SERVER] Path is %s \n\r",getSecondArgument(command));
       sendFileNames(command,&t,&r);

     }

     else if(equalsCommand(command) == CD){

        printf("[SERVER] Command is %s\n\r",command);
        printf("[SERVER] Second argument is %s\n\r",getSecondArgument(command));
        sendACK(&t);
        cd(getSecondArgument(command));
     }

     else if(equalsCommand(command) == CP){

        printf("[SERVER] Command is %s\n\r",command);
        printf("[SERVER] Second argument of cp is %s\n\r",getSecondArgument(command));

        sendACK(&t);
        sendFileSize(getSecondArgument(command), &t);
        sendFile(getSecondArgument(command), &t, &r);

     }

     else if(equalsCommand(command) == SN){
        printf("[SERVER] Command is %s\n\r",command);
        printf("[SERVER] Second argument of sn is %s\n\r",getSecondArgument(command));

        char filename[MSGSIZE];

        sendACK(&t);
        createFile(getSecondArgument(command),filename);
        int filesize = receiveFileSize(&r);
        sendACK(&t);
        writeInFile(filename ,filesize , &r , &t);
     }

     else if(equalsCommand(command) == EXIT){
        printf("[SERVER] Command is %s\n\r",r.payload);
        sendACK(&t);
        break;
     }

    } //endof else

	}

	printf("[SERVER] Finished receiving..\n");

	return 0;
}

 int equalsCommand(char * payload){
      char copy[MSGSIZE]; // strtok destroys the original data
      memcpy(copy , payload ,MSGSIZE);
      char * current = strtok (copy," ");
     // printf("[SERVER] processing token %s\n\r",current);
      if(strcmp (current , "ls") == 0 )
         return LS ;
      else if(strcmp(current , "cd") == 0 )
        return CD ;
      else if(strcmp(current , "sn") == 0 )
        return SN ;
      else if(strcmp(current , "exit") == 0 )
        return EXIT;
      else if(strcmp(current , "cp") == 0 )
        return CP;
  return -1 ;
 }


 int getNumberOfFiles(DIR * dir){
    int numberOfFiles = 0 ;
    struct dirent * entity ;
    while ((entity = readdir (dir)) != NULL){
      numberOfFiles++ ;
//      printf("[SERVER] filename is %s\n\r",entity->d_name);
    }
    closedir(dir);
    printf("[SERVER] number of files function call returns %d\n\r",numberOfFiles);
    return numberOfFiles;
 }



 char * getSecondArgument(char * commandRequest){
   char copy[MSGSIZE];
   memcpy(copy,commandRequest ,MSGSIZE);
   char * current = strtok (copy," ");
   current = strtok (NULL," ");
     // printf("[SERVER] processing token %s\n\r",current);

   return current ;
 }


 int sendACK(msg *t){
    memset(t->payload, 0 , MSGSIZE);
    sprintf(t->payload , "ACK");
    t->len = 3+1;
    int res = send_message(t);
      if(res > 0){
        printf("[SERVER] Sending ACK successfull\n\r");
      }else{
        printf("[SERVER] Sending ACK was not successfull\n\r");
      }
    return res ;
 }

 void sendNumberOfFiles(msg *t, char* path){

    DIR * dir = opendir(path);
    if(dir == NULL)
      return ; // cannot open the file

    int numberOfFiles = getNumberOfFiles(dir);

    if(TYPE == PARITY){
      sprintf(t->payload +1  ,"%d",numberOfFiles);
      t->payload[0] = checkParity(t);
      t->len = strlen(t->payload +1 ) + 1;

    }else if (TYPE == NOPARITY){
      sprintf(t->payload ,"%d",numberOfFiles);
      t->len = strlen(t->payload) + 1;
    }

    int res = send_message(t);
    if(res > 0){
        printf("[SERVER] Sending NOF successfull\n\r");
    }else{
        printf("[SERVER] Sending NOF was not successfull\n\r");
    }
 }

 void receiveACK(msg *r){
    int res = recv_message(r);
    if(res < 0 )
        printf("[SERVER] Error receiving message\n\r");
    else
        printf("[SERVER] ACK received succcessfully\n\r");
 }

 void sendFileNames(char* command, msg *t , msg *r){
    memset(t->payload , 0 , MSGSIZE);
    memset(r->payload , 0 , MSGSIZE);
    //printf("[SERVER] Command is %s\n\r",command);
    DIR * dir = opendir(getSecondArgument(command));
    int numberOfFiles = getNumberOfFiles(dir);
    dir = opendir(getSecondArgument(command));
    struct dirent * entity;
    int res;

      do{
        entity = readdir(dir);
        memset(t->payload , 0 , MSGSIZE);
        if(TYPE == PARITY){
           memcpy(t->payload +1 , entity->d_name , strlen (entity->d_name) + 1);
           t->len =  strlen(entity->d_name) +2 ;
           t->payload[0] = checkParity(t);
        }else{
            memcpy(t->payload , entity->d_name , strlen (entity->d_name) +1);
            t->len = strlen(entity->d_name) +1 ;
        }

        printf("[SERVER] Message payload is %s\n\r",t->payload);
        res =  send_message(t);
        if(res < 0)
            printf("[SERVER] Error sending %s\n\r",t->payload);
        else
            printf("[SERVER] Message sent %s\n\r",t->payload);
        if(receiveResponse(r) == ACK){
           printf("[SERVER] Received ACK \n\r");
        }else{
           printf("[SERVER] Received NACK \n\r");
        }
        numberOfFiles--;
    }while(numberOfFiles >0);
     closedir(dir);
 }


 void cd(char * path){
    chdir(path);
 }

 void sendFile(char * name , msg * t , msg * r ){
    char buff[MSGSIZE];
    int fd = open ( name , O_RDONLY ) ;
    int citite = 0 ;

    if(TYPE == NOPARITY){
    while((citite = read(fd,buff,MSGSIZE)) > 0 ){
       memset(t , 0 , MSGSIZE );
       memcpy(t->payload , buff , citite);
       t->len = citite;

       int response = send_message(t);
       if(response > 0){
         //printf("[SERVER] Successfully sent file segment %s\n\r",t->payload);
       }else{
          printf("[SERVER] Error sending file segment \n\r");
       }
       response = recv_message(r);

       if(response > 0){
          //printf("[SERVER] Successfully received file segment %s\n\r",r->payload);
       }else{
          printf("[SERVER] Error receiving file segment \n\r");
       }
    }
    }else if(TYPE == PARITY){

    while((citite = read(fd,buff, MSGSIZE - 1 )) > 0 ){
        memset(t , 0 , MSGSIZE);
        memcpy(t->payload+1,buff, citite);
        t->len = citite + 1 ;
        t->payload[0] = checkParity(t);

       int response = send_message(t);
       if(response > 0){
         //printf("[SERVER] Successfully sent file segment %s\n\r",t->payload);
       }else{
          printf("[SERVER] Error sending file segment \n\r");
       }
       response = recv_message(r);

       if(response > 0){
          //printf("[SERVER] Successfully received file segment %s\n\r",r->payload);
       }else{
          printf("[SERVER] Error receiving file segment \n\r");
       }
    }
    }

 }

  void sendFileSize(char * path , msg *t){
    FILE * file = fopen (path ,"r");
    printf("[SERVER] Path to the file is %s\n\r",path);
    if(file == NULL)
       printf("[SERVER] Cannot open file \n\r");
    fseek (file,0,SEEK_END);

    if(TYPE == PARITY){
      sprintf(t->payload + 1 , "%ld",ftell(file));
      t->len = strlen(t->payload) + 2 ;
      t->payload[0] = checkParity(t);
    }else{
      sprintf(t->payload ,"%ld",ftell(file));
      t->len = strlen(t->payload) +1;
    }
    printf("[SERVER] The size of the file is %ld\n\r",ftell(file));
    printf("[SERVER] Size of the message is %s\n\r",t->payload);
    int response = send_message(t);

       if(response > 0)
          printf("[SERVER] Successfully sent file size segment\n\r");
       else
          printf("[SERVER] Error sending file size segment \n\r");

    fclose(file);
  }

 void createFile(char * path , char * returnValue){
      char new_path[MSGSIZE] = "new_";
      int fd = open(strcat(new_path,path), O_CREAT|O_RDWR , 0666);
      if(fd != 0){
        printf("[SERVER] Created file successfully \n\r");
        memcpy(returnValue , new_path , strlen(new_path) +1 );
        close(fd);
      }else{
        printf("[SERVER] Error when creating file \n\r");
      }
 }

  int receiveFileSize(msg * r){

     int response = recv_message(r);
     msg t;

     while(checkParity(r) != getParity(r->payload)){
            printf("[SERVER] Corruption receiving file size \n\r");
            sendNACK(&t);
            recv_message(r);
     }

     if(response > 0)
         printf("[SERVER] Successfully received file size for sn\n\r");
      else{
         printf("[SERVER] Error receiving file size for sn \n\r");
       return -1 ;
     }
     if(TYPE == PARITY){
      printf("[SERVER] File size for sn is %d\n\r",atoi(r->payload + 1));
        return atoi(r->payload + 1) ;
     }
     printf("[SERVER] File size for sn is %d\n\r",atoi(r->payload));
     return atoi(r->payload);
  }



   void writeInFile(char * path ,int nrBytes, msg * r , msg *t){
    printf("[SERVER] write file path is  %s\n\r",path);
    printf("[SERVER] write file nrBytes is %d \n\r",nrBytes);

     int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);

     if(TYPE == NOPARITY){
       while(nrBytes > 0 ){
          int res = recv_message(r);
          if(res > 0){
            printf("[SERVER] Successfully received file part for sn\n\r");
            printf("[SERVER] Payload is %s\n\r",r->payload);
            printf("[SERVER] Size of the payload is %ld\n\r",strlen(r->payload));
            sendACK(t);
            write(fd , r->payload, r->len);
            nrBytes -= r->len;
            printf("[SERVER] %d bytes to come \n\r",nrBytes);
          }else{
            printf("[SERVER] Error receiving file part for sn \n\r");
        }
     }
     }else if (TYPE == PARITY){
        while(nrBytes > 0 ){

          while(checkParity(r) != getParity(r->payload)){
            printf("[SERVER] Error receiving file part \n\r");
            sendNACK(t);
            recv_message(r);
          }
             printf("[SERVER] Successfully received file part for sn \n\r");
             printf("[SERVER] Payload is %s\n\r",r->payload + 1);
             printf("[SERVER] Size of the payload is %d\n\r",r->len - 1);
             recv_message(r);
             sendACK(t);
             if(nrBytes < 1399 ) r->len = nrBytes +1 ;
             write(fd, r->payload + 1 , r->len - 1);
             nrBytes -= (r->len -1) ;
             printf("[SERVER] %d bytes to come \n\r",nrBytes);
        }
     }
     close (fd);
  }

  char getParity(char * payload ){
       return payload[0];
  }


  char checkParity(msg * r){
    int i = 0 ;
    char returnChar = 0 ;

	  for (i = 1 ; i < r->len ; i++){
        returnChar ^= r->payload[i];
 		}

    char returnBit = 0;
    returnBit = (returnChar >> 0) & 1;
    for(i= 1 ; i < 8 ; i++){
      returnBit ^= (returnChar >> i) & 1;
    }

    return returnBit ;
  }



  void sendNACK(msg *t){
    memset(t->payload, 0 , MSGSIZE);
    sprintf(t->payload , "NACK");
    t->len = 4+1;
    int res = send_message(t);
      if(res > 0){
        printf("[SERVER] Sending NACK successfull\n\r");
      }else{
        printf("[SERVER] Sending NACK was not successfull\n\r");
      }
  }

  int whatTestIsIt(char ** argv){
      if(argv[1] == NULL)
         return NOPARITY;
      if(strcmp (argv[1],"parity") == 0){
           return PARITY;
      }
      else if(argv[1] == NULL)
          return NOPARITY;
      return NOPARITY;
  }


   int receiveResponse(msg * r ){
    if(recv_message(r) > 0 ){
      if(TYPE == PARITY){
         if(strcmp(r->payload +1 , "ACK") == 0 )
            return ACK;  // transmission was successfull
         else if(strcmp (r->payload +1 , "NACK" ) == 0 ){
            return NACK; //retransmit
         }
      }else if (TYPE == NOPARITY){
          return ACK ;
      }
    }
    return ACK;
   }
