#include <SoftwareSerial.h>
#include <dht.h>
#define NodeID 5
#define DHT22_PIN 7

SoftwareSerial mySerial(2,3); //使用软串口，避免和程序烧写用的串口冲突，重定义pin2是rx，pin3是tx，实现多串口使用
String nodeSendString = ""; //节点发送数据包
String nodeGetString = ""; //节点接收数据包
String nodeData = ""; //传感器数据包（上传到服务器）
String tempString = ""; //暂时字符串变量
dht DHT;//传感器对象
int firstSend = 0; //第一次发送数据时随机选择信道发送即可
char SF[6] = {'7','8','9','A','B','C'}; //可以使用的SF扩频因子
String CH[6] = {"1C578DE0","1C5A9B20","1C5DAB60","1C60B5A0","1C63C2E0","1C66D020"}; //可以使用的CH频道
int chsfMatrix[6][6] = {{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0},{0,0,0,0,0,0}}; //横轴是SF,纵轴是CH
unsigned long time;
int ID = NodeID; //节点编号
int sendMessage = 0; //统计发送数据包总数
int getMessage = 0; //统计接收数据包的总数
float timeCost = 0; //计算传输速率
int sendYet = 0;//判断是否发送，用于计算等待某信道等了多长时间
int waitTime = 0;//等待某信道等了多长时间
int nowCH = 0;
int nowSF = 0;

void setup() {
  //定义串口波特率为9600
  Serial.begin(9600);
  mySerial.begin(9600);
  //确定是几号节点
  String firstSend = String("This is ") + String(NodeID) + String(" speaking!");
  Serial.println(firstSend);
  Serial.println("LIBRARY VERSION: " + String(DHT_LIB_VERSION));
  Serial.println("Type,\tstatus,\tHumidity (%),\tTemperature (C)");
  delay(3*1000); //方便时间计算
}

void loop() {
  mySerial.listen(); //开启软串口监听

  //限制发送传感器数据的频率
  int getTime = millis();
  if(getTime%(4*1000) == 0){
    String sensor = getSensorData();
    nodeSendToServer(sensor); //向服务器发送数据
//    testSend(sensor);
  } 

  //读取串口数据，使用readString连续读取字符串
  while(Serial.available() > 0 ){
     nodeSendString = Serial.readString();
     nodeSendString.trim(); // 去除字符串中的空格、回车、tab等
  }
  //向lora模块发送数据
  if(nodeSendString != ""){
    Serial.println(nodeSendString);
    if(nodeSendString == "+++")
      mySerial.print(nodeSendString); //进入配置模式不需要回车，命令：+++
    else if(nodeSendString == "send"){
//      testSend(getSensorData());
    }
    else
      mySerial.println(nodeSendString); //其他的AT指令都必须要回车，数据的话暂定需要回车，如：查询配置参数指令：AT+CFG?，直接输入字符即可，不需要再输入回车
  }
  
  
  //读取串口数据，使用readString连续读取字符串
  while(mySerial.available() > 0 ){
    nodeGetString = mySerial.readString();
    nodeGetString.trim(); // 去除字符串中的空格、回车、tab等
  }
  //获得lora模块返回的数据
  if(nodeGetString != "")
//  Serial.println(nodeGetString);
  if(nodeGetString[0] == char(ID + 48)){
    nodeHandleServerLMAC2(nodeGetString); //处理服务器回传数据
  }

  //当节点发送/接收的数据不为空时，须在循环的末端归零
  if(nodeSendString != "" || nodeGetString != ""){
    nodeSendString = "";
    nodeGetString = "";
  }
  int timeNow = millis();
  //当没发送成功sendYet == 0时就要开始计时等待多久
  if((timeNow %(1*1000) == 0) && (sendYet == 0)){
    waitTime++;
//    Serial.println("waitTime:"+String(waitTime));
    if(waitTime <= 4){
      chsfMatrix[nowCH][nowSF] = 1;
    }
    else if(waitTime > 4 && waitTime <= 8){
      chsfMatrix[nowCH][nowSF] = 2;
    }
    else if(waitTime > 8 && waitTime <= 12){
      chsfMatrix[nowCH][nowSF] = 3;
    }
    else if(waitTime > 16 && waitTime <= 20){
      chsfMatrix[nowCH][nowSF] = 4;
    }
    else if(waitTime > 20){
      chsfMatrix[nowCH][nowSF] = 5;
    }

//    for (int i = 0; i < 2; i++){
//      for(int j = 0; j < 2; j++){
//        Serial.print(chsfMatrix[i][j]);
//      }
//    }
//    Serial.println();
  }
  sendYet = 0;
  //每5秒随机清空信道的拥堵等级，防止当所有信道等级一样时都在卷1，1信道
  int timeToFree = millis();
  if(timeToFree % (20*1000) == 0){
    randomSeed(analogRead(6));
    int ranSF = random(0,1); //随机信道对应的SF
    int ranCH = random(0,2);
    chsfMatrix[ranCH][ranSF] = 0;
  }
  
}

//解析收到的传感器数据 函数
String getSensorData(){
  String data = "";
//  Serial.print("DHT22, \t");
  int chk = DHT.read22(DHT22_PIN);  //读取数据
  switch (chk)
  {
    case DHTLIB_OK:
//                Serial.print("OK,\t");
                break;
    case DHTLIB_ERROR_CHECKSUM:
                Serial.print("Checksum error,\t");
                break;
    case DHTLIB_ERROR_TIMEOUT:
                Serial.print("Time out error,\t");
                break;
    default:
                Serial.print("Unknown error,\t");
                break;
  }
  data = String(round(DHT.humidity)) + String(round(DHT.temperature)); //返回湿度+温度
//  Serial.println("humidity:"+String(round(DHT.humidity))+",temprature:"+String(round(DHT.temperature)));
  return data;
}

//选择最优的CH/SF信道，发送数据 函数
//发送数据格式为:节点ID+"数据"+CH/SF+时间
void nodeSendToServer(String sensorData){
  sendMessage++;
  String serverData;
  //首先随机选择一个CH/SF信道发送一次数据
  if(firstSend == 0){
    randomSeed(analogRead(5));
    int r = random(0,2);
    int randomSF = random(0,2); //随机信道对应的SF
    int randomCH = random(0,2);  //随机信道对应的CH
    String serverData;
    mySerial.listen(); //开启软串口监听
    //进入lora模块的配置模式修改CH/SF
    mySerial.print("+++");//每个指令之间尽量延时20毫秒，否则容易出现+++AT...而进入不了配置模式
    delay(50);
    mySerial.println("AT+TSF=0"+String(SF[randomSF]));//指令之间最后都有50演示，不容易出现错误
    delay(50);
    mySerial.println("AT+TFREQ="+CH[randomCH]);
    delay(50);
    mySerial.println("ATT");
    delay(50);
    while(mySerial.available() > 0 ){
      serverData = mySerial.readString();
    }//把mySerial的缓存清空，因为现在里面攒的是AT指令的答复，不来自服务器
    delay(50);
    //组装数据包
    unsigned long tempTime = millis();
    nodeData = String(ID) + sensorData + String(randomCH) + String(randomSF) + String(tempTime/1000);
    mySerial.println(nodeData); //使用lora节点发送数据
    firstSend = 1;
  }
  else{
    int bestSignal = 5; //最优信道，初始化为最大延迟等级，方便查找最优信道
    int bestSF = 0; //最优信道对应的SF
    int bestCH = 0;  //最优信道对应的CH
    //遍历信道占用矩阵，找到等级最小/最优的信道
    for(int k = 0; k < 2;k++){
      for(int l = 0; l < 2;l++){
        if(chsfMatrix[k][l] <= bestSignal){
          bestSignal = chsfMatrix[k][l];
          bestCH = k;
          bestSF = l;
        }
      }
    }
    nowCH = bestCH;
    nowSF = bestSF;
    //进入lora模块的配置模式修改CH/SF
    mySerial.print("+++");
    delay(50);
    mySerial.println("AT+TSF=0"+String(SF[bestSF]));
    delay(50);
    mySerial.println("AT+TFREQ="+CH[bestCH]);
    delay(50);
    mySerial.println("ATT");
    while(mySerial.available() > 0 ){
      serverData = mySerial.readString();
    }
    delay(50);
    //组装数据包
    unsigned long tempTime = millis();
    nodeData = String(ID) + sensorData + String(bestCH) + String(bestSF) + String(tempTime/1000);
    mySerial.println(nodeData); //使用lora节点发送数据
    
    //读取串口数据，使用readString连续读取字符串
//    while(mySerial.available() > 0 ){
//      serverData = mySerial.readString();
//      serverData.trim(); // 去除字符串中的空格、回车、tab等
//    }
//    nodeHandleServerLMAC2(serverData); //处理服务器回传数据
  }
}

//void testSend(String sensorData){
//  int bestSignal = 0; //最优信道，延迟等级最小
//  int bestSF = 0; //最优信道对应的SF
//  int bestCH = 0;  //最优信道对应的CH
//  String serverData;
//  //遍历信道占用矩阵，找到等级最小/最优的信道
//  bestSF = 4;
//  bestCH = 0;
//  //进入lora模块的配置模式修改CH/SF
//  mySerial.print("+++");
//  delay(50);
//  mySerial.println("AT+TSF=0"+String(SF[bestSF]));
//  delay(50);
////  mySerial.println("AT+TFREQ="+CH[bestCH]);
//  delay(50);
//  mySerial.println("ATT");
//  while(mySerial.available() > 0 ){
//    serverData = mySerial.readString();
//  }
////  Serial.println(serverData);
//  delay(50);
//  //组装数据包
//  unsigned long tempTime = millis();
//  nodeData = String(ID) + sensorData + String(bestCH) + String(bestSF) + String(tempTime/1000);
//  mySerial.println(nodeData); //使用lora节点发送数据
//  Serial.println(nodeData);
//}

//服务器回传数据处理，更新本地信道占用矩阵 函数(LMAC-2版本)
//服务器回传数据的格式为:节点ID+"ANSWERED"+CH/SF+时间(发送数据的时间）
void nodeHandleServerLMAC2(String serverData){
  if(int(serverData[0] - 48) == ID){
    int ID = NodeID;
    int revCH = int(serverData[9] - 48);
    int revSF = int(serverData[10] - 48);
    getMessage++;
    
    unsigned long time1 = serverData.substring(11,serverData.length()).toInt();
    unsigned long time2 = millis()/1000;
    unsigned long timeStamp = time2 - time1;
//    Serial.print("time1:"+String(time1)+",time2:"+String(time2));
    timeCost += timeStamp;
    timeCost = timeCost/getMessage;
//    if(time2 % 120 == 0){
      String transmitRateAndTxRx = "Node:" + String(ID);
      transmitRateAndTxRx += ", transmitRate:" + String(timeCost);//传输速率
      transmitRateAndTxRx += ", tx:" + String(sendMessage) + ", rx:" + String(getMessage);
      transmitRateAndTxRx += ", CH:" + String(revCH) + ", SF:" +String(revSF);
      Serial.println(transmitRateAndTxRx);
//      mySerial.println(transmitRateAndTxRx);
//    }
  }

  sendYet = 1;//数据已通过节点模块发送
  waitTime = 0;//将等待时间归零
}
