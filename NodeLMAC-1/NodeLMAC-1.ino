#include <SoftwareSerial.h>
#include <dht.h>
#define NodeID 4
#define DHT22_PIN 7

SoftwareSerial mySerial(2,3); //使用软串口，避免和程序烧写用的串口冲突，重定义pin2是rx，pin3是tx，实现多串口使用
String nodeSendString = ""; //节点发送数据包
String nodeGetString = ""; //节点接收数据包
String nodeData = ""; //传感器数据包（上传到服务器）
String tempString = ""; //暂时字符串变量
dht DHT;//传感器对象
char SF[6] = {'7','8','9','A','B','C'}; //可以使用的SF扩频因子
String CH[6] = {"1C578DE0","1C5A9B20","1C5DAB60","1C60B5A0","1C63C2E0","1C66D020"}; //可以使用的CH频道，CH/SF各自选择2个，共四个信道，共五个节点争夺
unsigned long time;
int ID = NodeID; //节点编号
int sendMessage = 0; //统计发送数据包总数
int getMessage = 0; //统计接收数据包的总数
float timeCost = 0; //计算传输速率
int randomSF = 0; //随机信道对应的SF
int randomCH = 0;  //随机信道对应的CH
int lastTx = 0;
int lastRx = 0;
int lossRateOutput = 1;

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

  randomSeed(analogRead(5));
  int r = random(0,2);
  randomSF = random(0,2); //随机信道对应的SF
  randomCH = random(0,2);  //随机信道对应的CH
  String serverData;
  mySerial.listen(); //开启软串口监听
  //进入lora模块的配置模式修改CH/SF
  mySerial.print("+++");
  delay(13);
  mySerial.println("AT+TSF=0"+String(SF[randomSF]));
  delay(13);
  mySerial.println("AT+TFREQ="+CH[randomCH]);
  delay(13);
  mySerial.println("ATT");
  delay(13);
  while(mySerial.available() > 0 ){
    serverData = mySerial.readString();
  }
  delay(13);
}

void loop() {
  mySerial.listen(); //开启软串口监听

  //限制发送传感器数据的频率，每2秒发送一次
  int getTime = millis();
  if(getTime%(2*1000) == 0){
    String sensor = getSensorData();
//    nodeSendToServer(sensor); //向服务器发送数据
    nodeSendToServer(sensor);
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
      nodeSendToServer(getSensorData());
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
    nodeHandleServerLMAC1(nodeGetString); //处理服务器回传数据
  }

  //当节点发送/接收的数据不为空时，须在循环的末端归零
  if(nodeSendString != "" || nodeGetString != ""){
    nodeSendString = "";
    nodeGetString = "";
  }

//  unsigned long timeNow = millis()/1000;
//  if(timeNow % 120 == 0){
//      String transmitRateAndTxRx = "Node:" + String(ID);
//      transmitRateAndTxRx += ", transmitRate:" + String(timeCost);
//      transmitRateAndTxRx += ", tx:" + String(sendMessage) + ", rx:" + String(getMessage);
////      transmitRateAndTxRx += ", CH:" + String(nowCH) + ", SF：" +String(nowSF);
//      Serial.println(transmitRateAndTxRx);
//      mySerial.println(transmitRateAndTxRx);
//    }
  
//  int time2 = millis();
//  if(time2 % (10*1000) == 0 && lossRateOutput == 1){
//    float lossRate = 100 - ((getMessage-lastRx)*100)/(sendMessage - lastTx);
//    Serial.println("lossRate:"+ String(lossRate) + "%");
//    lastTx = sendMessage;
//    lastRx = getMessage;
//    lossRateOutput = 0;
//  }
  
   
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

//随机选择CH/SF信道，发送数据 函数
//发送数据格式为:节点ID+"数据"+CH/SF+时间
void nodeSendToServer(String sensorData){
  sendMessage++;
  //组装数据包
  unsigned long tempTime = millis();
  nodeData = String(ID) + sensorData + String(randomCH) + String(randomSF) + String(tempTime/1000);
  mySerial.println(nodeData); //使用lora节点发送数据
}


//服务器回传数据处理 函数(LMAC-1版本)
//服务器回传数据的格式为:节点ID+"ANSWERED"+CH/SF+时间（发送数据的时间）
void nodeHandleServerLMAC1(String serverData){
  if(int(serverData[0] - 48) == ID){
    getMessage++;
    int nowCH = int(serverData[9] - 48);
    int nowSF = int(serverData[10] - 48);
    unsigned long time1 = serverData.substring(11,serverData.length()).toInt();
    unsigned long time2 = millis()/1000;
    unsigned long timeStamp = time2 - time1;
    timeCost += timeStamp;
    timeCost = timeCost/getMessage;
//    if(time2 % 120 == 0){
      String transmitRateAndTxRx = "Node:" + String(ID);
      transmitRateAndTxRx += ", transmitRate:" + String(timeCost);
      transmitRateAndTxRx += ", tx:" + String(sendMessage) + ", rx:" + String(getMessage);
      transmitRateAndTxRx += ", CH:" + String(nowCH) + ", SF:" +String(nowSF);
      Serial.println(transmitRateAndTxRx);
//      mySerial.println(transmitRateAndTxRx);
//    }
  }
//    lossRateOutput = 1;
}
