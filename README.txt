Name: Dan Do
USD ID: 5349927407

--------------------------------------------------------------------------------------------------------------------------
Project Description:
-------------------------------------------------------------

This project implements a simplified blockchain system using TCP and UDP socket programming in C++.

Note: Extra credit NOT completed

--------------------------------------------------------------------------------------------------------------------------
Compile Steps:
-------------------------------------------------------------

Compile:
make all

Clean:
make clean

--------------------------------------------------------------------------------------------------------------------------
Code Files:
-------------------------------------------------------------
1. serverM.cpp / serverM.h: 

Implements the Main Server.

Responsibilities:
Accepts TCP connections from clients.
Accepts TCP connections from the monitor.
Communicates with backend servers using UDP.
Computes wallet balances.
Validates transactions.
Generates transaction serial numbers.
Appends new transactions to a randomly selected backend server.
Generates txchain.txt for TXLIST requests.
-------------------------------------------------------------
2. serverA.cpp / serverA.h

Implements Backend Server A.

Responsibilities:
Reads transaction records from block1.txt.
Returns transaction records to the Main Server.
Appends new encrypted transactions to block1.txt.
-------------------------------------------------------------
3. serverB.cpp / serverB.h

Implements Backend Server B.

Responsibilities:
Reads transaction records from block2.txt.
Returns transaction records to the Main Server.
Appends new encrypted transactions to block2.txt.
-------------------------------------------------------------
4. serverC.cpp / serverC.h

Implements Backend Server C.

Responsibilities:
Reads transaction records from block3.txt.
Returns transaction records to the Main Server.
Appends new encrypted transactions to block3.txt.
-------------------------------------------------------------
5. client.cpp / client.h

Implements the client application.

Responsibilities:
Sends CHECK WALLET and TXCOINS requests.
Receives and displays responses from the Main Server.
-------------------------------------------------------------
6. monitor.cpp / monitor.h

Implements the monitor application.

Responsibilities:
Sends TXLIST requests.
Receives confirmation from the Main Server.
-------------------------------------------------------------
7. common.h

Additional header file that contains shared utility functions used by all programs.

Responsibilities:
Encryption and decryption functions.
Transaction parsing functions.
File processing functions.
Balance calculation functions.
TCP helper functions.
UDP helper functions.
Transaction data structure definitions.


--------------------------------------------------------------------------------------------------------------------------
Message Formats:
-------------------------------------------------------------
Client -> Main Server

CHECK WALLET request:
CHECK|<username>
Example:
CHECK|Martin

TXCOINS request:
TX|<sender>|<receiver>|<amount>
Example:
TX|Martin|Chinmay|20
-------------------------------------------------------------
Monitor -> Main Server

TXLIST request:
TXLIST
-------------------------------------------------------------
Main Server -> Backend Servers

Request all transactions:
GET_ALL

Append transaction:
APPEND|<encrypted transaction>

Example:
APPEND|10 Pduwlq Fklqpdb 53
-------------------------------------------------------------
Backend Servers -> Main Server

Transaction records are returned as newline-separated entries.

Example:
1 Udfkhdo Mrkq 78
6 Udfkhdo Dolfh 72

Append confirmation:
OK
-------------------------------------------------------------
Main Server -> Client

Balance result:
The current balance of <username> is : <balance> txcoins.

Successful transaction:
<sender> successfully transferred <amount> txcoins to <receiver>.

The current balance of <sender> is : <balance> txcoins.

Insufficient balance:
<sender> was unable to transfer <amount> txcoins to <receiver> because of insufficient balance.

The current balance of <sender> is : <balance> txcoins.

User not found:
<username> is not a part of the network.
or
Unable to proceed with the transaction as <user> is not part of the network.
-------------------------------------------------------------
Main Server -> Monitor

TXLIST confirmation:
Successfully received a sorted list of transactions from the main server.



--------------------------------------------------------------------------------------------------------------------------
Idiosyncrasies / Known Limitations:
-------------------------------------------------------------
The Main Server assumes that all transaction serial numbers in the block files are unique and contiguous.
The code assumes that the block files exist and are formatted correctly.
The input arguments are case-sensitive.
For the code to function properly, the servers must be started in this order before running any client or monitor commands: serverM, serverA, serverB, serverC

The project will fail if:
A required block file is missing.
The transaction files are manually modified into an invalid format.