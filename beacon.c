#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

//Global variable for target port
//Defines buffer size
int TARGET_PORT;
#define BUFFER 4096
#define inputFile "Project5datafile.txt"

//Prototype to remove warning
int usleep(useconds_t useconds);

//Function which just gets the character count of an integer
int getNumDigitOffset(int digit)
{
    int page = digit;
    int count = 0;
    
    while(page != 0)
    {
        page /= 10;
        ++count;
    }
    
    return count;
}
 
int main(int argc, char **argv)
{

//Check for arguments passed are valid
if((argc >= 1 && argv[1] <= 0) || strtod(argv[1], NULL) <= 0)
{
    printf("INPUT ERROR: When running program input integer value equal to group number.\n");
    return -1;
}

//Gets group number input of user
int input = atoi(argv[1]);

//Sets target port variable
//Generates the IPUSED variable with the multicast group IP
TARGET_PORT = 22200 + input;
char IPUSED[11];
sprintf(IPUSED,"239.10.5.%d",input);
printf("Targetport used: %d \n",TARGET_PORT);
printf("IP used: %s \n",IPUSED);

//Socket variable
int SC;

//Creates the socket connection and checks for errors
if ((SC = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0) 
{
    printf("SOCKET ERROR! \n");
    return -1;
}

//Sets the group multicast IP and target port by the variables declared
struct sockaddr_in address;
socklen_t LAddress = sizeof(address);
memset((char *)&address, 0, sizeof(address));
address.sin_addr.s_addr = inet_addr(IPUSED);
address.sin_port = htons(TARGET_PORT);
address.sin_family = AF_INET;

//Sets the TTL to be 30 in order to avoid loops and broadcast within designated subnet
int intFS = 30;

//Sets multicast socket
if (setsockopt( SC, IPPROTO_IP, IP_MULTICAST_TTL, (char*) &intFS, sizeof(intFS)) < 0 )
{
    perror("Multicast socket error");
    return -1;
}
//Attemps to read file
FILE *f;
f=fopen(inputFile,"r");
    
if (f == NULL)
{
    printf("Cannot open file \n");
    exit(0);
}

//Gets the character count of the file
char i;
i = fgetc(f);
int getMaxData = 0;
while (i != EOF)
{
    i = fgetc(f);
    getMaxData += 1;
}
fclose(f);
//Divides the character count of the file with the buffer to know exactly how many chunks of data is needed to store the entire file
int dataCOM = getMaxData/(BUFFER + 1);
printf("Num packages: %d \n",dataCOM + 1);

//Prints a message notifying user of success
//Sets first message number to be 1
//creates buffer CM for message to be broadcasted
//msg char array for entire package size
printf("Server running! \n");
int msgCount = 1;
char CM[BUFFER  + 1];
char msg[BUFFER*2];
int size;
FILE *fd;
//snprintf(buff, BUFFER + 1, "FT! %d$", dataCOM);
//Loops and broadcasts each time the message to multicast group
//Opens and starts reading the file

    while(1)
    {
        //Do loop which will read the file until message content read is less than the message buffer size
        fd = fopen(inputFile, "r");
        do {
            //Reads each character in the size of the buffer specified
            size = fread(CM, 1, sizeof(CM), fd);
            if (size <= 0)
            {
                printf("%s\n",CM);
                break;
            }
            //Stores message and message number in msg buffer
            //Sends message to clients
            snprintf(msg, BUFFER*2,"%s>%d",CM,msgCount);
            if(sendto(SC, msg, sizeof(msg), 0, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                printf("ERROR: Message failed to send! \n");
                return -1;
            }
            else
            {
                
            }
            //Stores just the number of messages expected and sends it to clients
            snprintf(msg, getNumDigitOffset(dataCOM) + 2,"$%d",dataCOM + 1);
            if(sendto(SC, msg, sizeof(msg), 0, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                printf("ERROR: Message failed to send! \n");
                return -1;
            }
            else
            {
                
            }
            //increments to next message
            msgCount += 1;
            
            //Can be removed or altered for faster/slower file transfer
            //If propagation delay is not ~0 between client and beacon then removing usleep will cause transfer issues
            usleep(5000);
            
        } while (size == sizeof(CM));
        
        //Gets the rest of the file if not EOF
        //Cuts the file off at the EOF to prevent looping of read data
        if(size > 0)
        {
            int cut = size;
            fread(CM, 1, size, fd);
            CM[cut + getNumDigitOffset(msgCount) + 1] = 0;
            snprintf(msg, cut + getNumDigitOffset(msgCount),"%s>%d",CM,msgCount);
            if(sendto(SC, msg, sizeof(msg), 0, (struct sockaddr *)&address, sizeof(address)) < 0)
            {
                printf("ERROR: Message failed to send! \n");
                return -1;
            }
            else
            {
                
            }
        }
        //Resets msg count after EOF found and closes the file read
        msgCount = 1;
        size = 0;
        fclose(fd);
    }
    
        
}
