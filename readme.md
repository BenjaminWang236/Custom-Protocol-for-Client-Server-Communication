# Programming Assignment #2: Custom Protocol on top of UDP for Client-Server communication for server-side access permission identification

COEN 233 Introduction to Computer Networks (SCU W2022)

Author: Benjamin Wang (1179478)

---
## About this Project

Client using customized protocol on top of UDP protocol for requesting identification from server for access permission to the cellular network.

Client `myclient` send Access Permission requests, also known as Identification requests, to server using custom `subscriber_packet` set to Access Permission request mode

Server `myserver` reads in the access permission request from Client `myclient` and check the source subscriber number (phone number) and technology type (2G - 5G) against the verification database read from `./input_files/verification_database.txt` that also contains the Paid status of the subscriber. 

Server `myclient` respond to the Client `myclient` based on the verification result with 

1. Subscriber Not Paid
2. Subscriber Not Exist
3. Subscriber Access Granted

---
## Project Structure
Main program files and executables are in the project root folder, containing this `readme`, `myclient`, `myserver`, `customProtocol`, `testing`, and `Makefile` source codes and compiled executables

`examples` folder has the UDP-server-client tutorial codes I based my programming assignment off of

`input_files` contain the input files used for testing myclient and myserver's verification database

`output_files` contain the output files generated from myclient and myserver executions, their converted PDF version, and finally screenshots within a sub-folder `screenshots`

`instruction` contain the instruction docx and PDFs for COEN 233 Programming Assignment 02

---
## How to Compile the Code
Use the provided Makefile to compile the code, preferrably after clearing compiled executables first.
```C
make
```
or better yet:
```C
make clean && make
```
Note that verbose debugging mode can be activated by not commenting out line 18 of customProtocol.h: `// #define DEBUGGING 0` before compilation

Individual compilation options are also available within the `Makefile`

## How to Run the Code
Open two terminals on the same computer. By default Port number `8080` and host "`localhost`" are used. Navigate to project's main folder first!

---
### Run Server
```C
./myserver
```
Alternatively myserver also supports specifying port number or port number and verification database filename together:
```C
./myserver 8080
```
or,
```C
./myserver 8080 ./input_files/verification_database.txt
```
Note that since this simple implementation of myserver runs the while-loop indefinitely, the user will have to quit myserver by keyboard interrupt such as
```
Ctrl + C
```
This also means you can't redirect the command line outputs to an output-file, so that have to be done manually

---
### Run Client
Must give an input file as argument:
```C
./myclient ./input_files/access_permission_requests.txt
```
To save output to an output file, preferrably in the `output_files` folder,
```C
./myclient ./input_files/access_permission_requests.txt > ./output_files/client_output.txt 
```

---
