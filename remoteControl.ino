/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com  
*********/

#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>
#include <MPU6050_tockn.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>

#define I2C_SLAVE_ADDR 0x04


//OLED Paramerters

/* Uncomment the initialize the I2C address , uncomment only one, If you get a totally blank screen try the other*/
#define i2c_Address 0x3c //initialize with the I2C addr 0x3C Typically eBay OLED's
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define OLED_RESET -1   //   QT-PY / XIAO

Adafruit_SH1106G display = Adafruit_SH1106G(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//mpu
MPU6050 mpu6050(Wire);

// Replace the next variables with your SSID/Password combination
const char* ssid = "b12raspberrypi";
const char* password = "raspberrypi";                

// Add your MQTT Broker IP address, example:
//const char* mqtt_server = "192.168.1.144";
const char* mqtt_server = "169.254.132.244";

WiFiClient espClient;
PubSubClient client(espClient);
long lastMsg = 0;
char msg[50];
int value = 0;

float temperature = 0;
float humidity = 0;


// motor parameters

int x = 0;    //left speed
int y = 0;    //right speed
int z = 92;     //steering angle
int rear = 0;   //distance from back
int angle = 0;  //imu angle





void setup() {
  
  Wire.begin();
  Serial.begin(115200);
  display.begin(i2c_Address, true);

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SH110X_WHITE);


  //display booting text on oled display
  for (int i = 0; i <= 5; i++) {
    display.setCursor(0, 0);
    display.print("EEEbot booting");
    display.display();
    for (int j = 0; j <= 3; j++) {
      delay(100);
      display.print(".");
      display.display();
      delay(100);
    }
    display.clearDisplay();
  }
  


  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  


  pinMode(ledPin, OUTPUT);
  mpu6050.begin();
  mpu6050.calcGyroOffsets(true);




  

}

void drive(int leftMotor, int rightMotor, int angle){
  
  Wire.beginTransmission(I2C_SLAVE_ADDR);

  Wire.write((byte)((leftMotor & 0x0000FF00) >> 8));  
  Wire.write((byte)(leftMotor & 0x000000FF));         

  Wire.write((byte)((rightMotor & 0x0000FF00) >> 8));  
  Wire.write((byte)(rightMotor & 0x000000FF));         

  Wire.write((byte)((angle & 0x0000FF00) >> 8));  
  Wire.write((byte)(angle & 0x000000FF));

  Wire.endTransmission();
  delay(100);


}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* message, unsigned int length) {
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;
  
  for (int i = 0; i < length; i++) {
    Serial.print((char)message[i]);
    messageTemp += (char)message[i];
  }
  Serial.println();

  // Feel free to add more if statements to control more GPIOs with MQTT

  // If a message is received on the topic esp32/output, you check if the message is either "on" or "off". 
  // Changes the output state according to the message
  // if (String(topic) == "esp32/output") {
  //   Serial.print("Changing output to ");
  //   if(messageTemp == "on"){
  //     Serial.println("on");
  //     digitalWrite(ledPin, HIGH);
  //   }
  //   else if(messageTemp == "off"){
  //     Serial.println("off");
  //     digitalWrite(ledPin, LOW);
  //   }
  // }
  // if( String(topic)== "esp32/KongMove"){
  //   if(messageTemp == "w")
  //   {
  //     x = 100;
  //     y = 100;
  //   }
  //   if(messageTemp == "s"){
  //     x = -100;
  //     y = -100;
  //   }
  //   if(messageTemp !="s" && messageTemp!="w"){
  //     x =0;
  //     y =0;
  //   }

    
    

  // }
  if( String(topic)== "esp32/KongSteer"){
    if(messageTemp == "a")
    {
      z = 70;
    }
    if(messageTemp == "d"){
      z = 110;
    }
    if(messageTemp == ""){
      z = 92;
    }
    

  

  }
  
  if( String(topic)== "esp32/OLEDoutput"){

    display.setCursor(0,0);
    display.println(messageTemp);
    display.display();
    display.clearDisplay();    
  }

  

  
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP8266Client")) {
      Serial.println("connected");
      // Subscribe
      client.subscribe("esp32/output");
      client.subscribe("esp32/KongSteer");
      client.subscribe("esp32/KongMove");
      client.subscribe("esp32/OLEDoutput");
      
    } 
    else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      x= 0;
      y =0;
    }
  }
}
void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  //mpu update
  mpu6050.update();

  drive(x,y,z);

  // if(PS4.isConnected()){
  //   if(PS4.LStickX()>=10){
  //     z = 92 + PS4.LStickX();
  //   }
  //   if(PS4.LStickX()<=-10){
  //     z =92 - PS4.LStickX();
  //   }

  // }


  long now = millis();
  if (now - lastMsg > 5000) {

    // display.clearDisplay();

    // display.setCursor(0,0);
    
    // display.setTextSize(1);
  

    lastMsg = now;
    
    // Temperature in Celsius
    temperature = mpu6050.getTemp();
    // Uncomment the next line to set temperature in Fahrenheit 
    // (and comment the previous temperature line)
    //temperature = 1.8 * bme.readTemperature() + 32; // Temperature in Fahrenheit
    
    // Convert the value to a char array
    char tempString[8];
    char speed[8];
    char direction[8];
    dtostrf(temperature, 1, 2, tempString);
    dtostrf(x,1,0,speed);
    dtostrf(z,1,0,direction);

    // display.print("Temperature: ");
    // display.println(tempString);
    // display.print("Speed:");
    // display.println(speed);
    // display.print("Direction:");
    // display.println(direction);
    // display.display();
    client.publish("esp32/temperature", tempString);

    
  }
}
