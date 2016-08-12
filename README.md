MetCap is a packet sniffer and soft-tap developed to mirror packets flowing through an Ethernet interface. 
It is purely based on the NDIS which allows us to sniff packets and reflects them to the configured Kafka server acting as a soft-tap. 
MetCap consist of two components,

    1. NDIS 6.3 based protocol driver.
    2. User mode soft-tap.

These two act in tandem to create a soft-tap, where the protocol driver sniffs the traffic and delivers to the user mode application. 
Whereas the user mode application does the redirection logic and writes the packets to the configured kafka server.

Please read the description.htm for more details.
