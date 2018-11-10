#include <Wire.h>
#include <LCD.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>         //載入軟體串列埠函式庫
#include <DHT.h>                  //載入 DHT11 函式庫
#include <pitch.h>
#define DHTPIN 2                   //定義 DIO 腳 2 為 DHT11 輸入
#define DHTTYPE DHT11              //定義 DHT 型態為 DHT11

DHT dht(DHTPIN, DHTTYPE);         //初始化 DHT11 感測器
SoftwareSerial sSerial(7, 8);       //設定軟體串列埠腳位 RX, TX為 DIO 腳 7, 8
LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // 設定 LCD I2C 位址

int led = 13;                      // 定義 13腳位為LED輸入
int fan = 3;                       // 定義 3腳位接繼電器
int spk = 4;                       // 定義 4腳位接蜂鳴器

void setup() {
  pinMode(led, OUTPUT);             //設定led為輸出
  pinMode(fan, OUTPUT);             //設定繼電器為輸出
  pinMode(spk, OUTPUT);          
  digitalWrite(spk, LOW);           //初始蜂鳴器為不鳴叫 
         
  Serial.begin(9600);                //啟始硬體串列埠 (除錯用)
  sSerial.begin(9600);              //啟始軟體串列埠 (與 ESP8266 介接用)
  dht.begin();                      //啟始 DHT11 溫濕度感測器
  lcd.begin(16,2);
  String Welcome = "Gjun Simen No.1";
  lcd.clear();
  lcd.leftToRight();
  lcd.print(Welcome);
  sSerial.println("AT+RST");        //軟體串列埠傳送 AT 指令重啟 ESP8266
  Serial.println("Waiting for Strating ESP8226 20 Seconds");
  delay(20000);                   //ESP8266載入時間差 20秒
  Serial.println("OK, Now Starting to get information for Temperature & Humidity...");

}
  
void loop() {
  float h = dht.readHumidity();       //讀取濕度
  float t = dht.readTemperature();    //讀取攝氏溫度
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Humidity: ");
  lcd.print(h);
  lcd.print("%");
  lcd.setCursor(0, 1);
  lcd.print("Temp:");
  lcd.print(t);
  lcd.print("oC");

// 如果溫度高於 24度，亮燈，發出警告聲，啟動繼電器
  if (t > 24) {
    digitalWrite(led, HIGH);
    digitalWrite(fan, HIGH);
    digitalWrite(spk, LOW);
  }
  else {
    digitalWrite(led, LOW);
    digitalWrite(fan, LOW);
    digitalWrite(spk, HIGH);
  }
  
  //有任何一個是 NAN 就不往下執行資料傳送
  if (isnan(h) || isnan(t)) {
    Serial.println("Failed to read from DHT sensor!");
    return;
  }
  String param = "t=" + (String)t + "&h=" + (String)h; //製作參數字串


  //與 SQL Server 主機建立 TCP 連線
  String cmd = "AT+CIPSTART=\"TCP\",\"114.33.237.11\",80";
  sSerial.println(cmd); //向 ESP8266 傳送 TCP 連線之 AT 指令

  //偵測 TCP 連線是否成功
  if (sSerial.find("Error")) {
    Serial.println("AT+CIPSTART error!");
    return;  //連線失敗跳出目前迴圈 (不做後續傳送作業)
  }
  Serial.println(cmd);  //輸出 AT 指令於監控視窗

  //製作 GET 字串
  String GET = "GET /arduino/add.php?" + param + "\r\n";
  Serial.println(GET);  //顯示 GET 字串內容於監控視窗
  cmd = "AT+CIPSEND=" + String(GET.length()); //傳送 GET 字串長度之 AT 指令
  sSerial.println(cmd);  //告知 ESP8266 即將傳送之 GET 字串長度
  Serial.println(cmd); //輸出 AT 指令於監控視窗

  //檢查 ESP8266 是否回應
  if (sSerial.find(">")) {  //若收到 ESP8266 的回應標頭結束字元
    Serial.print("Sending Data to MySQL Server");
    sSerial.print(GET);  //向 ESP8266 傳送 GET 字串內容
  }
  else {  //沒有收到 ESP8266 回應
    sSerial.println("AT+CIPCLOSE");  //關閉 TCP 連線
    Serial.println("AT+CIPCLOSE");    //顯示連線關閉訊息於監控視窗
  }
  delay(30000);  //延遲 30 秒 
}


