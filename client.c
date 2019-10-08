#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <time.h>
#include <netinet/in.h>
#include <unistd.h>

//Sets tarbet port and message buffer size
//Buffer size needs to be same as beacon buffer size
//Will only use Buffer/2 to store each message rest is ignored
#define TARGET_PORT 22207
#define BUFFER 8192

//Prototype to remove library warning when compiling code
int isdigit(int arg);

//Global variabls
//Package storage variable & variables to hold sizes and package numbers
char **orderedPackages;
int page;
int arraySize;
int currentProgress;
int lossCheck = 0;

//Function which parses the broadcasted message and returns the message number
int parseMessageAndRetrieveValue(char *str)
{
    //Gets the message, stores all integers found in size variable
    char *current = str;
    int size = 0;
    while (*current)
    {
            if (isdigit(*current))
            {
                size += strtol(current, &current, 10);
            }
            else
            {
                current++;
            }
        
    }
    //Returns the value found in the message
    return size;
}

//Function which checks for package loss during each iteration
void checkForPackageLoss(int currentMessageNumber)
{
    //Expects the next message to be N + 1
    if(lossCheck + 1 == currentMessageNumber)
    {
        
    }
    else if(lossCheck + 1 < currentMessageNumber)
    {
        printf("Beacon package loop detected! \n");
    }
    else
    {
        if(lossCheck + 1 < arraySize)
        {
            printf("Package loss found! Expected package #: %d Recieved package #: %d \n", lossCheck + 1, currentMessageNumber);
        }
    }
    
    lossCheck = currentMessageNumber;
}

//Function which parses the message retrieved by the client
//Gets the actual message and then the message ordering
//Stores the message in the package array in the correct position
void findArrayLoc(char *str,int *packageM)
{
    int init_size = strlen(str);
    char delim[] = ">";
    
    char *ptr = strtok(str, delim);
    char *text;
    char *page;
    int key = 0;
    
    while (ptr != NULL)
    {
        if(key == 0)
        {
            text = ptr;
            key = 1;
        }
        else
        {
            page = ptr;
        }
        
        ptr = strtok(NULL, delim);
    }
    
    
    int loc = parseMessageAndRetrieveValue(page);
    checkForPackageLoss(loc);
    
    //Malloc prevention method implemented causes end text buffer issues with file transfered(appends text after EOF),
    //if given more time could remove need to malloc everytime package was recieved, reducing memory usage and CPU usage
    
    //if(packageM[loc - 1] == 0)
    //{
        packageM[loc - 1] = 1;
        char *result = (char *)malloc(strlen(text)+1);
        strcpy(result,str);
        orderedPackages[loc - 1] = result;
        free(ptr);
        printf("Datagram with order #: %d recieved. Putting item in array location: %d \n",loc,loc - 1);
        printf("Progress: %d out of %d packages recieved \n",currentProgress + 1,arraySize);
    //}
}

int main(int argc, char **argv) {

//Char buffer for message recieved from Beacon
unsigned char charBuffer[BUFFER + 1];
//Socket Connection variables
int SC;
int RM;

//Creates the UDP socket connection
if ((SC = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
{   
    perror("SOCKET ERROR! \n");
    return -1;
}

//Creates the address variable and creates the size to store the IP address
struct sockaddr_in address;
socklen_t LAddress = sizeof(address);
memset(&address, 0, sizeof(address));

//Sets the IP address and port
address.sin_addr.s_addr = htonl(INADDR_ANY);
address.sin_port = htons(TARGET_PORT);
address.sin_family = AF_INET;

//Variables which checks for loss as well as displays the calculatons for loss
int lossCheck = 0;
double numLossDetected = 0;
double msgsRec = 0;
int lossCompare = 0;
char parseInput[BUFFER];
time_t currentT;
struct tm * timeNow;
    
//Bind the socket
if(bind(SC, (struct sockaddr*)&address, sizeof(address)))
{
    perror("Binding datagram socket error");
    close(SC);
    return -1;
}
else
{
    printf("Binding datagram to socket success.\n");
}

//Multicast setup, gets the group IP
//Sets socket
struct ip_mreq mreq;
mreq.imr_multiaddr.s_addr = inet_addr("239.10.5.7");
mreq.imr_interface.s_addr = htonl(INADDR_ANY);

if(setsockopt(SC, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof(mreq)) < 0)
{
    perror("ERROR: Connection on multicast failed! \n");
    close(SC);
    return -1;
}
else
{
    printf("Connected to multicast group. \n");
}

//First retrieves the total size in chunks specified the file will be
while(1)
{
    RM = recvfrom(SC, charBuffer, BUFFER, 0, (struct sockaddr *)&address, &LAddress);
    
    if (RM > 0)
    {
        if(charBuffer[0] == '$')
        {
            arraySize = parseMessageAndRetrieveValue((char*) charBuffer);
            break;
        }
    }
    else
    {
        //If message wasn't recieved print error
        perror("ERROR: No message recieved! \n");
    }
}
//Gets the message size and creates enough storage for all packages to be retrieved
int arrayBuffer = (BUFFER/2)+1;
int packageMap[arrayBuffer];
orderedPackages = (char **)malloc((arraySize * (arrayBuffer) * sizeof(char)) + 1);
//Loop which continously recieves messages broadcasted from the multicast group
//Uses recvfrom to wait until all packages are retrieved
while(1)
{
    
    RM = recvfrom(SC, charBuffer, BUFFER, 0, (struct sockaddr *)&address, &LAddress);
    if (RM > 0)
    {
        //Makes sure the size message is not being read
        if(charBuffer[0] == '$')
        {
            
        }
        else
        {
            //Stores message in array
            findArrayLoc((char *)charBuffer,packageMap);
            int shouldExit = 0;
            currentProgress = 0;
            int i = 0;
            //Checks if all message have been retrieved, if so exits program after creating outputfile
            while(i < arraySize)
            {
                if(orderedPackages[i] == NULL)
                {
                    shouldExit = 1;
                    
                }
                else
                {
                    currentProgress += 1;
                }
                i++;
            }

            //Ran only if all messages have been retrieved
            if(shouldExit == 0)
            {
      
                if (remove("clientOutput.txt") == 0)
                {
                    
                }
                    
                FILE *outputFile = fopen("clientOutput.txt", "a");
                
                if(outputFile == 0)
                {
                    perror("ERROR: Couldn't create output file! \n");
                }
                i = 0;
                //Removes unnecessary space at end of file and puts guaranteed EOF
                orderedPackages[arraySize - 1][strlen(orderedPackages[arraySize - 1]) - 1] = '\0';
                orderedPackages[arraySize - 1][strlen(orderedPackages[arraySize - 1]) - 1] = '\0';
                
                while(i < currentProgress)
                {
          
                    int results = fputs(orderedPackages[i], outputFile);
                    
                    if (results == EOF)
                    {
                        perror("ERROR: Couldn't write message recieved to file! \n");
                    }
                    
                    i++;
                }
                
    
                printf("Retrieved all data, disconnecting from beacon! \n");
                break;
            }
        }
    }
    else
    {
        //If message wasn't recieved print error
        perror("ERROR: No message recieved! \n");
    }
    
}
    //Close socket connection
    close(SC);
    

return 0;
}

