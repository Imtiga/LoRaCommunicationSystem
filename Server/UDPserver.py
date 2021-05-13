from socket import *
UDPserverPort = 12000
UDPserverSocket = socket(AF_INET, SOCK_DGRAM)
UDPserverSocket.bind(('', UDPserverPort))
print ('The UDPserver is ready to receive')
while True:
    UDPmessage, clientAddress = UDPserverSocket.recvfrom(2048)
    sensorDataMessage = "hasdiufhdsiu"
    UDPserverSocket.sendto(sensorDataMessage.encode(),clientAddress)
