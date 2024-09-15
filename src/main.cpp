#include <Arduino.h>
#include <WiFi.h>
#include <SPIFFS.h>
#include <CAN.h>
#include "soc/soc.h"          // Disable brownout detector
#include "soc/rtc_cntl_reg.h" // Required for RTC_CNTL_BROWN_OUT_REG


void writeFile(const char * path, String message);
String readFile(const char * path);
void printFile();
uint32_t fileCrcCal();
void flashingFile();
void webClientHandling();
void vehicleControl(uint32_t direciton, uint32_t speed);
void dataToOled();


const char* ssid = "NGUYENDUCSAOLAU1.2G";
const char* password = "28247467";
// const char* ssid = "Nguyen Thinh";
// const char* password = "Nguyentramy2021@";

String html_main_page = "<!DOCTYPE html>"
              "<html lang='en'>"
              "<head>"
              "<meta charset='UTF-8'>"
              "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
              "<title>Option</title>"
              "<style>"
              "  body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; }"
              "  .button { width: 200px; height: 100px; font-size: 24px; margin: 10px; cursor: pointer; }"
              "</style>"
              "</head>"
              "<body>"
              "<button class='button' onclick=\"window.location.href='/upload_file';\">Flashing</button>"
              "<button class='button' onclick=\"window.location.href='/control';\">Vehicle control</button>"
              "</body>"
              "</html>";


String html_request_upload ="<!DOCTYPE html>\
<html>\
<body>\
  <h2>File Upload</h2>\
  <form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">\
    <input type=\"file\" name=\"upload\">\
    <input type=\"submit\" value=\"Upload\">\
  </form>\
</body>\
</html>\
";

String html_confirm_upload_ok ="<!DOCTYPE html>\
<html>\
<body>\
  <h2>File Upload</h2>\
  <form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">\
    <input type=\"file\" name=\"upload\">\
    <input type=\"submit\" value=\"Upload\">\
  </form>\
  <hr>\
  <h2>File Upload OK</h2>\
</body>\
</html>\
";

String html_confirm_upload_failed ="<!DOCTYPE html>\
<html>\
<body>\
  <h2>File Upload</h2>\
  <form method=\"POST\" action=\"/upload\" enctype=\"multipart/form-data\">\
    <input type=\"file\" name=\"upload\">\
    <input type=\"submit\" value=\"Upload\">\
  </form>\
  <hr>\
  <h2>File Upload Failed</h2>\
</body>\
</html>\
";

String vehicle_control_page ="<!DOCTYPE html> <html> <head> <title>Mini Car Control</title> <style> body { display: flex; justify-content: center; align-items: center; height: 100vh; margin: 0; background-color: #f0f0f0; font-family: Arial, sans-serif; } .gameboy { border: 2px solid #222; border-radius: 10px; background-color: #ccc; width: 200px; /* height: 200px; */ padding: 10px; box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.3); } .gameboy-space { /* border: 2px solid #222; */ border-radius: 10px; /* background-color: #ccc; */ width: 50px; padding: 10px; /* box-shadow: 0px 0px 10px rgba(0, 0, 0, 0.3); */ } .button-container { display: flex; justify-content: space-between; margin: 10px 0; } .button { width: 45px; height: 45px; background-color: #fc7070; border: none; border-radius: 50%; color: #fff; /* font-size: 16px; */ cursor: pointer; box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.3); } .button-space { width: 20px; height: 45px; /* background-color: #444; */ border: none; border-radius: 50%; /* color: #fff; */ /* font-size: 16px; */ cursor: pointer; /* box-shadow: 0px 0px 5px rgba(0, 0, 0, 0.3); */ } .slider-container { margin: 10px 0; } .slider { width: 100%; -webkit-appearance: none; appearance: none; height: 15px; border-radius: 5px; background-color: #64c4ff; outline: none; opacity: 0.7; -webkit-transition: .2s; transition: opacity .2s; } .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 25px; height: 25px; border-radius: 50%; background: #ff2828; cursor: pointer; } .slider::-moz-range-thumb { width: 25px; height: 25px; border-radius: 50%; background: #444; cursor: pointer; } </style> </head> <body> <div class=\"gameboy\"> <div class=\"button-container\"> <div class=\"button-space\"></div> <button class=\"button\" onmousedown=\"sendCommand('up')\"></button> <div class=\"button-space\"></div> </div> <div class=\"button-container\"> <button class=\"button\" onmousedown=\"sendCommand('left')\"></button> <div class=\"button-space\"></div> <button class=\"button\" onmousedown=\"sendCommand('right')\"></button> </div> <div class=\"button-container\"> <div class=\"button-space\"></div> <button class=\"button\" onmousedown=\"sendCommand('down')\"></button> <div class=\"button-space\"></div> </div> </div> <div class=\"gameboy-space\"> </div> <div class=\"gameboy\"> <div class=\"slider-container\"> <input type=\"range\" min=\"0\" max=\"100\" value=\"50\" class=\"slider\" id=\"speedSlider\" oninput=\"updateSpeed()\"> </div> </div> <script> function sendCommand(command) { var xhr = new XMLHttpRequest(); xhr.open('GET', '/' + \"direction?\" + command, true); xhr.send(); } function updateSpeed() { var speedSlider = document.getElementById('speedSlider'); var speed = speedSlider.value; if (speed % 10 == 0) { var xhr = new XMLHttpRequest(); xhr.open('GET', '/' + \"speed?\" + speed, true); xhr.send(); } } </script> </body> </html>";

WiFiServer server(80);



void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  
  Serial.begin(115200);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

  server.begin();

  // start the CAN bus at 500 kbps
  if (!CAN.begin(1000E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }

  // Initialize SPIFFS
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }
}



void loop() {
  Serial.println(".");
  delay(100);

  webClientHandling();

  dataToOled();

  if (Serial.available() > 0)
  {
    printFile();
    flashingFile();
    // Serial.println();
    // Serial.println(fileCrcCal(), HEX);

    while((Serial.available() > 0))
    {
      Serial.read();
    }
  }
}



void writeFile(const char * path, String message) {
  File file = SPIFFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written successfully");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}


String readFile(const char * path) {
  String ret;
  File file = SPIFFS.open(path, FILE_READ);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  
  while (file.available()) {
    ret = file.readString();
  }
  file.close();

  return ret;
}


void printFile()
{
    Serial.println();
    String file_content_from_memory = readFile("/bin_file.bin");
    uint32_t file_from_memory_size = file_content_from_memory.length();

    for(uint32_t i = 0; i < file_from_memory_size; i++)
    {
      Serial.print(file_content_from_memory[i], HEX);
      Serial.print('\t');
      if ((i % 16) == 0)
      {
        Serial.println();
      }
    }
    Serial.println();
}

uint32_t fileCrcCal()
{
    uint32_t crc = 0xFFFFFFFF;  // Initial CRC value
    uint32_t CRC_POLYNOMIAL = 0x04C11DB7;
    
    String file_content_from_memory = readFile("/bin_file.bin");
    uint32_t file_from_memory_size = file_content_from_memory.length();

    for(uint32_t i = 0; i < file_from_memory_size; i+=4)
    {
      uint32_t word =  (uint32_t)(*(uint32_t*)(file_content_from_memory.c_str() + i));

      crc ^= word;
      for (int j = 0; j < 32; j++) {
          if (crc & 0x80000000) {
              crc = (crc << 1) ^ CRC_POLYNOMIAL;
          } else {
              crc <<= 1;
          }
      }
    }
    return crc ^ 0xFFFFFFFF;  // Final XOR value
}


void flashingFile()
{
    // Reset message
    uint8_t reset_message[] = {0x07, 0x11, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    CAN.beginPacket(0x719);
    CAN.write(reset_message, sizeof(reset_message));
    CAN.endPacket();    
    
    delay(500);  

    // Write firmware size
    uint8_t wdbi_1001_message[] = {0x07, 0x2E, 0x10, 0x01, 0x55, 0x55, 0x55, 0x55};
    uint32_t firmware_size = String(readFile("/bin_file.bin")).length();
    Serial.print("firmware_size: ");
    Serial.println(firmware_size);
    wdbi_1001_message[4] = (firmware_size >> 0) & 0xFF; 
    wdbi_1001_message[5] = (firmware_size >> 8) & 0xFF; 
    wdbi_1001_message[6] = (firmware_size >> 16) & 0xFF; 
    wdbi_1001_message[7] = (firmware_size >> 24) & 0xFF; 
    CAN.beginPacket(0x719);
    CAN.write(wdbi_1001_message, sizeof(wdbi_1001_message));
    CAN.endPacket();    
    
    delay(500);  

    // Write firmware crc
    uint8_t wdbi_1002_message[] = {0x07, 0x2E, 0x10, 0x02, 0x55, 0x55, 0x55, 0x55};
    uint32_t firmware_crc = fileCrcCal();
    Serial.print("firmware_crc: ");
    Serial.println(firmware_crc, HEX);
    wdbi_1002_message[4] = (firmware_crc >> 0) & 0xFF; 
    wdbi_1002_message[5] = (firmware_crc >> 8) & 0xFF; 
    wdbi_1002_message[6] = (firmware_crc >> 16) & 0xFF; 
    wdbi_1002_message[7] = (firmware_crc >> 24) & 0xFF; 
    CAN.beginPacket(0x719);
    CAN.write(wdbi_1002_message, sizeof(wdbi_1002_message));
    CAN.endPacket();    
    
    delay(500);  

    // Flashing request message
    uint8_t flashing_request_message[] = {0x07, 0x34, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    CAN.beginPacket(0x719);
    CAN.write(flashing_request_message, sizeof(flashing_request_message));
    CAN.endPacket();
    
    delay(15);  

    int received_packet_size = CAN.parsePacket();
    // if ((received_packet_size == 8) && (CAN.packetId() == 0x791))
    if (true)
    {
      String response_message;
      while (CAN.available()) 
      {
        response_message += (char)CAN.read();
      }
      // if (response_message[1] == 0x74)
      if (true)
      {
        String file_content_from_memory = readFile("/bin_file.bin");
        uint32_t file_from_memory_size = file_content_from_memory.length();



        Serial.println(file_from_memory_size);
        for(uint32_t i = 0; i < file_from_memory_size; i+=4)
        {
          CAN.beginPacket(0x719);
          CAN.write(0x08); CAN.write(0x36); CAN.write(0x08);
          CAN.write(file_content_from_memory[i]);
          CAN.write(file_content_from_memory[i+1]);
          CAN.write(file_content_from_memory[i+2]);
          CAN.write(file_content_from_memory[i+3]);
          CAN.write(0x55);
          CAN.endPacket();
    
          delay(20);  

          // int received_packet_size = CAN.parsePacket();
          // if ((received_packet_size == 8) && (CAN.packetId() == 0x791))
          // {
          //   String response_message;
          //   while (CAN.available()) 
          //   {
          //     response_message += (char)CAN.read();
          //   }
          // }
        }


        // Flashing exit message
        uint8_t flashing_exit_message[] = {0x07, 0x37, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
        CAN.beginPacket(0x719);
        CAN.write(flashing_exit_message, sizeof(flashing_exit_message));
        CAN.endPacket();
      }
    }
}


void webClientHandling()
{

  WiFiClient client = server.available();
  if (client) 
  {        
      while(server.available());

      Serial.println("New Client request");

      String client_input = "";
      uint8_t i = 0;
      if (client.available())
      {
          client_input += client.readString();
      }
      // Serial.println(client_input);
      
      if (client_input.indexOf("/upload_file") != -1)
      {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.println(html_request_upload);
        client.println();
      }
      else if (client_input.indexOf("/upload") != -1)
      {
        uint32_t boundary_code_start_index = client_input.indexOf("boundary=") + String("boundary=").length();
        uint32_t boundary_code_stop_index = client_input.indexOf("\n", boundary_code_start_index);

        String boundary_code = client_input.substring(boundary_code_start_index, boundary_code_stop_index);

        uint32_t boundary_code_begin_start_index = client_input.indexOf(boundary_code, boundary_code_stop_index);

        String file_info = client_input.substring(boundary_code_begin_start_index);
        
        uint32_t file_name_start_index = file_info.indexOf("filename=\"") + String("filename=\"").length();
        uint32_t file_name_stop_index = file_info.indexOf("\"", file_name_start_index);

        String file_name = file_info.substring(file_name_start_index, file_name_stop_index);

        uint32_t file_content_start_index = file_info.indexOf("Content-Type: application/octet-stream") + String("Content-Type: application/octet-stream").length() + 4;
        uint32_t file_content_stop_index = file_info.length() - boundary_code.length() - 7; 

        String file_content = file_info.substring(file_content_start_index, file_content_stop_index);
        uint32_t file_size = file_content.length();
        
        writeFile("/bin_file.bin", file_content);
        String file_content_from_memory = readFile("/bin_file.bin");
        uint32_t file_from_memory_size = file_content_from_memory.length();

        // Serial.println("******************");
        // Serial.println(file_size);
        // for (int i = 0; i < file_size; i++)
        // {
        //   Serial.print(file_content[i], HEX);
        //   Serial.print("\t");
        //   if ((i%16) == 0)
        //   {
        //     Serial.println();
        //   }
        // }
        // Serial.println("******************");
        // Serial.println(file_from_memory_size);
        // for (int i = 0; i < file_from_memory_size; i++)
        // {
        //   Serial.print(file_content_from_memory[i], HEX);
        //   Serial.print("\t");
        //   if ((i%16) == 0)
        //   {
        //     Serial.println();
        //   }
        // }

        if ((file_content == file_content_from_memory) && (file_size == file_from_memory_size))
        {
          Serial.println("New Client request");
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println(html_confirm_upload_ok);
          client.println();
        }
        else
        {
          Serial.println("New Client request");
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println(html_confirm_upload_failed);
          client.println();
        }
        
        if (file_size > 0)
        {
          printFile();
          flashingFile();
        }
      }
      else if (client_input.indexOf("/control") != -1)
      {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();
          client.println(vehicle_control_page);
          client.println();

          uint32_t substring_start_index = client_input.indexOf("?") + 1;
          uint32_t substring_end_index = client_input.indexOf(" ", substring_start_index);
          String extracted_value = client_input.substring(substring_start_index, substring_end_index);
          // Serial.println(extracted_value);
          static uint8_t direction = 0; // 0:up, 1:right, 2:down, 3:left
          static uint8_t speed = 0;
          if (extracted_value == "up")
          {
            direction = 0;
          }
          else if (extracted_value == "right")
          {
            direction = 1;
          }
          else if (extracted_value == "down")
          {
            direction = 2;
          }
          else if (extracted_value == "left")
          {
            direction = 3;
          }
          else
          {
            speed = extracted_value.toInt();
          }
          vehicleControl(direction, speed);

          Serial.print("Vehicle Control: ");
          Serial.println(extracted_value);
      }
      else
      {
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.println(html_main_page);
        client.println();
      }

      Serial.println("Stop Client");
      client.stop();
  }
}


void vehicleControl(uint32_t direciton, uint32_t speed)
{
    CAN.beginPacket(0x201);
    CAN.write(direciton);
    CAN.write((speed >> 0) & 0xFF);
    CAN.write((speed >> 8) & 0xFF);
    CAN.endPacket();
}

void dataToOled()
{
  if (CAN.parsePacket() && (CAN.packetId() != -1)) 
  {
    if (CAN.packetId() == 0x200)
    {
      uint8_t data[8] = {0};
      uint8_t data_index = 0;
      while (CAN.available()) 
      {
        data[data_index] = CAN.read();
        data_index++;
      }

      uint32_t direciton = data[0];
      uint32_t speed = (((uint16_t)data[1]) << 8) | data[0];

      
    }
  }
}