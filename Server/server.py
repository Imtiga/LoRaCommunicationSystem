from socket import *
import time
import threading
serverport = 6001
serverSocket = socket(AF_INET,SOCK_STREAM) # 创建socket对象
serverSocket.bind(('192.168.199.193',serverport)) #将socket绑定到本机IP地址和端口
serverSocket.listen(1) # 服务器开始监听来自客户端的连接
print('The TCPserver is ready to receive!')

sensorData = [[10,10],[12,12],[13,14],[15,16],[12,17],[13,12]] #sensor[ID]记录ID对应的节点的湿度和温度,所以第0位不用管
channelCount = [[0,0,0,0,0,0],[0,0,0,0,0,0],[0,0,0,0,0,0],[0,0,0,0,0,0],[0,0,0,0,0,0],[0,0,0,0,0,0]] #统计每个信道使用次数
crowdLevel = [[1,1,1,1,1,1],[1,1,1,1,1,1],[1,1,1,1,1,1],[1,1,1,1,1,1],[1,1,1,1,1,1],[1,1,1,1,1,1]] #每个信道的拥挤等级
allCount = 0 # 统计服务器总共收到的信息条数，每200条广播一次全局信道占用率矩阵

# LMAC-1服务器数据处理函数
def serverHandleNodeLMAC1(data):
    nodeID = data[0]
    humidity = data[1:3]
    temprature = data[3:5]
    rest = data[5:]
    nowCH = data[5]
    noewSF = data[6]
    sensorData[int(nodeID)][0] = int(humidity)
    sensorData[int(nodeID)][1] = int(temprature)
    backData = nodeID + "ANSWERED" + rest
    return backData

# LMAC-2服务器数据处理函数
def serverHandleNodeLMAC2(data):
    nodeID = data[0]
    humidity = data[1:3]
    temprature = data[3:5]
    rest = data[5:]
    nowCH = data[5]
    noewSF = data[6]
    sensorData[int(nodeID)][0] = int(humidity)
    sensorData[int(nodeID)][1] = int(temprature)
    backData = nodeID + "ANSWERED" + rest
    return backData

# test LMAC-2
# tempdata = "1232534213"
# res = serverHandleNodeLMAC2(tempdata)
# print(res)
# print(sensorData)

# LMAC-3服务器数据处理函数
def serverHandleNodeLMAC3(data):
    global allCount
    allCount = allCount + 1
    nodeID = data[0]
    humidity = data[1:3]
    temprature = data[3:5]
    nowCH = data[5]
    nowSF = data[6]
    rest = data[7:]
    sensorData[int(nodeID)][0] = int(humidity)
    sensorData[int(nodeID)][1] = int(temprature)
    channelCount[int(nowCH)][int(nowSF)] = channelCount[int(nowCH)][int(nowSF)] + 1

    # 服务器每收到20条信息就对信道占用矩阵进行一次更新，统计在这20条中每个信道传递了几条信息，传递的越多，说明该信道占用率越高，等级就越高    
    if allCount == 20:
        allCount = 0
        for g in range(2):
            for h in range(2):
                if channelCount[g][h] <= 4:
                    crowdLevel[g][h] = 1
                elif channelCount[g][h] > 4 and channelCount[g][h] <= 6:
                    crowdLevel[g][h] = 2
                elif channelCount[g][h] > 6 and channelCount[g][h] <= 8:
                    crowdLevel[g][h] = 3
                elif channelCount[g][h] > 8 and channelCount[g][h] <= 10:
                    crowdLevel[g][h] = 4
                elif channelCount[g][h] > 10:
                    crowdLevel[g][h] = 5
                channelCount[g][h] = 0
        print(crowdLevel)

    # 组装数据
    matrixString = str(nodeID) + "ANSWERED" + nowCH + nowSF
    for i in range(2):
        for j in range(2):
            matrixString = matrixString + str(crowdLevel[i][j])
    matrixString = matrixString + rest
    return matrixString

#test LMAC-3
# tempdata = "1232534213"
# for i in range(200):
#     res = serverHandleNodeLMAC3(tempdata)
# print(sensorData)
# print(channelCount)
# print(crowdLevel)
# print(res)

#使用多线程的方式，防止UDPserver被TCPserver堵死
def UDPserver(sensordata,port):
    UDPserverPort = port
    UDPserverSocket = socket(AF_INET, SOCK_DGRAM)
    UDPserverSocket.bind(('', UDPserverPort))
    print ('The UDPserver is ready to receive!')
    while True:
        UDPmessage, clientAddress = UDPserverSocket.recvfrom(2048)
        sensorDataMessage = ""
        for i in range(6):
            for j in range(2):
                sensorDataMessage = sensorDataMessage + str(sensordata[i][j])
        # print(sensorDataMessage)
        UDPserverSocket.sendto(sensorDataMessage.encode(),clientAddress)

#创建并启动UDPserver线程
#进程线程是同步进行的，双方有任何一方被阻塞另一方也会被阻塞
# UDPthread = threading.Thread(target=UDPserver,args=(sensorData,12000))
# UDPthread.start()

whileCount= 0

while True:
    time.sleep(1.1) #arduino串口处理时间必须在1秒以上
    whileCount = whileCount + 1

    # 每当接收到客户端socket的请求时，该方法就返回对应的socket和远程地址
    ConnectionSocket,addr = serverSocket.accept()  

    nowData = ConnectionSocket.recv(1024).decode()
    nowData = nowData.strip()
    print(nowData)
    if(len(nowData) > 0):
        if(nowData[0] == '1' or nowData[0] == '2' or nowData[0] == '3' or nowData[0] == '4' or nowData[0] == '5'):
            if(len(nowData) >= 7):
                #LMAC-1
                # res = serverHandleNodeLMAC1(nowData)
                # ConnectionSocket.send(res.encode())

                #LMAC-2
                res = serverHandleNodeLMAC2(nowData)
                ConnectionSocket.send(res.encode())

                #LMAC-3
                # res = serverHandleNodeLMAC3(nowData)
                # ConnectionSocket.send(res.encode())
        
    ConnectionSocket.close()



    if whileCount%50 == 0:
        print("sensorData:",sensorData)
        print("channelCount:",channelCount)
        print("crowdLevel:",channelCount)
        whileCount = 0

'''
# 发送端测试
while True:
    time.sleep(1.1) #arduino串口处理时间必须在1秒以上
    whileCount = whileCount + 1
    # 每当接收到客户端socket的请求时，该方法就返回对应的socket和远程地址
    ConnectionSocket,addr = serverSocket.accept()
    backdata = "1ANSWERED04134"
    ConnectionSocket.send(backdata.encode())
    ConnectionSocket.close()

    #使用UDP与网页通信，本端作为服务器UDPserver，网页作为UDPclient
    #网页用于展示传感器数据
    UDPmessage, clientAddress = UDPserverSocket.recvfrom(2048)
    sensorDataMessage = ""
    for i in range(6):
        for j in range(2):
            sensorDataMessage = sensorDataMessage + str(sensorData[i][j])
    print(sensorDataMessage)
    UDPserverSocket.sendto(sensorDataMessage.encode(),clientAddress)
'''