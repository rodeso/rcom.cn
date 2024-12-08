# Computer Networks

## Evaluation

### Organization
 - Groups of 2 students
### Evaluation criteria
 - Participation during class (continuous evaluation)
 - Presentation and demonstration of the work
 - Individual 15-minute quiz to be answered in the classroom, on the last
class before the presentation
 - Final report
### Demonstration of the work
 - Replicate the network topology described in Part 2 / Exp 6
 - Using different IP addresses for the Bridges (announced before the
demonstration starts)

## Part 1: Development of a download application

### Develop application download ftp://ftp.netlab.fe.up.pt/pub/...
 - Application downloads a single file
 - Implements FTP application protocol, as described in RFC959
 - Adopts URL syntax, as described in RFC1738
ftp://[<user>:<password>@]<host>/<url-path>
### Steps
 - Experiments using Telnet application (Telnet, SMTP, POP, HTTP and FTP); focus on FTP
 - Specification/design of a download application
     - unique use case: connect, login host, passive, get path, success (file saved in CWD) or un-success (indicating failing phase)
     - challenging programming aspects: gethostbyname, sockets, control connection, passive, data connection
 - Implement a very simple FTP client at home
     - reuse existing programs: clientTCP.c, getIP.c
### Learning objectives
 - Describe client - server concept and its peculiarities in TCP/IP
 - Characterize application protocols in general, characterize URL, describe in detail the behaviour of FTP
 - Locate and read RFCs
 - Implement a simple FTP client in C language
 - Use sockets and TCP in C language
 - Understand service provided DNS and use it within a client program

### RFC959

RFC959 stands for "Request for Comments 959," which is a document that specifies the File Transfer Protocol (FTP). FTP is a standard network protocol used to transfer files from one host to another over a TCP-based network, such as the Internet.

RFC959 was published in October 1985 and defines the commands and responses used in FTP, as well as the data structures and transmission modes. It provides a detailed description of how clients and servers should interact to ensure reliable and efficient file transfers. The protocol supports various features, including user authentication, directory navigation, and file manipulation commands.

Core Features:
Commands and Responses: Defines a set of standard commands (like USER, PASS, STOR, RETR, etc.) and response codes (e.g., 200, 331, 550) for interaction between clients and servers.
 - Authentication: Supports user credentials (username and password) for access control.
 - Directory Navigation: Allows clients to navigate, list, and manage directories on the server.
 - Data Transfer Modes: Supports multiple modes, including ASCII and binary file transfers, and can transfer data in streams or blocks.
 - Connection Architecture: Separates the control connection (for commands and responses) from the data connection (for file transfers), providing flexibility and performance.


### RFC1738

RFC1738 stands for "Request for Comments 1738," which is a document that specifies the Uniform Resource Locators (URL) syntax. A URL is a web address that identifies a resource on the Internet, such as a webpage, file, or directory.

Published in December 1994, RFC1738 defines the syntax and components of a URL, including the protocol, host, port, path, and query parameters. It provides guidelines for constructing valid URLs and describes how they are used to access resources on the web.

Core Features:
 - URL Components: Describes the structure of a URL, including the protocol (e.g., http, ftp), host (e.g., www.example.com), port (optional), path (e.g., /path/to/resource), and query parameters (e.g., ?key1=value1&key2=value2).
 - URL Encoding: Specifies rules for encoding special characters in URLs to ensure compatibility with web browsers and servers.
 - URL Schemes: Defines standard URL schemes (e.g., http, ftp, mailto) and their corresponding protocols for accessing resources.
 - URL Syntax: Provides examples and guidelines for constructing valid URLs, including relative and absolute URLs.
 - URL Resolution: Describes how URLs are resolved by web browsers and servers to locate and retrieve resources on the Internet.

### Telnet

Use Telnet to interact with an FTP server:

Connect:
```bash
telnet ftp.netlab.fe.up.pt 21
```
Log in:
```bash
USER anonymous
PASS your_email@example.com
```
Switch to passive mode:
```bash
PASV
```
Retrieve a file:
```bash
RETR pub/somefile.txt
```
Observe the server responses and how connections are managed.



## Part 2: Configuration and Study of a Computer Network

Y = number of the group

### Experiment 1: Configure IP Network

#### Steps

 - Connect E1 TUX3 and E1 TUX4 to the switch 
     - gnuY3E1 and gnuY4E1 to any port on the switch
 - Configure eth1interface of tuxY3 and eth1 interface of tuxY4 using ifconfig and route commands
     - ifconfig eth1 172.16.Y0.1/24 on tuxY3
     - ifconfig eth1 172.16.Y0.254/24 on tuxY4
 - Open Wireshark and ping from tuxY3 to tuxY4 (ping 172.16.Y0.254)

00:c0:df:08:d5:9a

#### Questions

### Experiment 2: Implement two bridges in a switch

#### Steps

 - Connect and configure E1 of tuxY2 and register its IP and MAC addresses
     - gnuY2E1 to any port on the switch and gnuY2S0 to cisco, as well as the console
     - ifconfig eth1 172.16.Y1.1/24
     - open GTKTerm and change the baud rate to 115200
         ```bash
         > admin
         > /system reset-configuration
         > y
         ```
 - Create bridgeY0 and bridgeY1
     - create the two bridges
     ```bash
     > /interface bridge add name=bridgeY0
     > /interface bridge add name=bridgeY1
     ```
     - remove the ports from the default bridge
     ```bash
     > /interface bridge port remove [find interface=ether1] 
     > /interface bridge port remove [find interface=ether2]
     > /interface bridge port remove [find interface=ether3]
    ```
     - add the ports to the bridges
     ```bash
     > /interface bridge port add bridge=bridgeY0 interface=ether1
     > /interface bridge port add bridge=bridgeY0 interface=ether2
     > /interface bridge port add bridge=bridgeY1 interface=ether3
     ```

 - Start the capture at tuxY3.eth1
     - ping from tuxY3 to tuxY4 and tuxY2

#### Questions

### Experiment 3: Configure a Router in Linux

#### Steps

 - Make the Tux 4 a router
     - Ligar eth1 do Tux54 à porta 4 do switch. Configurar eth1 do Tux54
     ```bash
     ifconfig eth1 up
     ifconfig eth1 172.16.51.253/24
     ```


## Theory

### TCP/IP

 - IP (Internet Protocol) is implemented on all computers (hosts) and routers
 - Each computer has a unique IP address on each subnet it belongs to
 - Each process on a computer has a unique address (port)

### Demultiplexing

#### TCP/UDP header (port)
 - FTP → 21
 - Telnet → 23

#### IP header (protocol)
 - ICMP → 1
 - IGMP → 2
 - TCP → 6
 - UDP → 17
#### Ethernet header (type)
 - IP → 0x0800
 - ARP → 0x0806

#### IP Address

 - 32 bits
 - Network address
 - Host address
 - Subnet mask
 - Broadcast address

#### TCP

 - RFC 793
 - Characteristics
     - Ensures reliable end-to-end octet flow over unreliable support
     - Connection oriented protocol
     - Full-duplex connections
     - Positive Acknowledgment (ACK)
     - Recovers from losses and errors(retransmissions) after time-out
     - Orderly delivery of data to the application
     - Flow and congestion control
     - Multiplexing multiple TCP connections over the same IP address
 - TCP connection establishment
     - 3-way handshake
     - Client-server model

 - Source Port: port of origin
 - Destination Port: port of destination
 - Sequence Number: identifies, in the sender stream, the sequence of octets sent
 - Acknowledgement Number: corresponds to the octet number expected to receive
 - HLEN: the length of the TCP header (in 32-bit words)
 - URG: informs whether the Urgent Pointer field should be interpreted
 - ACK: informs whether the Ack number field is valid
 - PSH: allows to disable buffering
 - RST: used to reset a connection
 - SYN: allows to establish a connection
 - FIN: allows to close a connection