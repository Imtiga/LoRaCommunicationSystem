from socket import *
serverName = '127.0.0.1'
serverPort = 12000
clientSocket = socket(AF_INET, SOCK_DGRAM)
message = "Ask for data"
clientSocket.sendto(message.encode(),(serverName, serverPort))
dataMessage, serverAddress = clientSocket.recvfrom(2048)
print (dataMessage.decode())
clientSocket.close()
