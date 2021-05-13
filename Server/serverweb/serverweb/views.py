from django.http import HttpResponse
from django.shortcuts import render#导入render模块
from socket import *

#先定义一个数据列表，当然后面熟了可以从数据库里取出来
# list = [{"name":'good','password':'python'},{'name':'learning','password':'django'}]
sensorlist = [{'ID':'1','humidity':'40','temprature':'28'},
                {'ID':'2','humidity':'40','temprature':'28'},
                {'ID':'3','humidity':'40','temprature':'28'},
                {'ID':'4','humidity':'40','temprature':'28'},
                {'ID':'5','humidity':'40','temprature':'28'}]

def show(request):
    serverName = '127.0.0.1'
    serverPort = 12000
    clientSocket = socket(AF_INET, SOCK_DGRAM)
    message = "Ask for data"
    clientSocket.sendto(message.encode(),(serverName, serverPort))
    dataMessage, serverAddress = clientSocket.recvfrom(2048)
    print (dataMessage.decode())
    dataMsg = dataMessage.decode()
    clientSocket.close()
    dataMsg = dataMsg[4:]
    for i in range(5):
        if dataMsg == "":
            break
        sensorlist[i]['humidity'] = int(dataMsg[0])*10 + int(dataMsg[1])
        dataMsg = dataMsg[2:]
        sensorlist[i]['temprature'] = int(dataMsg[0])*10 + int(dataMsg[1])
        dataMsg = dataMsg[2:]

    # return HttpResponse("Hello world ! ")
    #通过render模块把index.html这个文件返回到前端，并且返回给了前端一个变量form，在写html时可以调用这个form来展示list里的内容
    return render(request,'showNode.html',{'form':sensorlist})


