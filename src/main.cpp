#include <Arduino.h>

#include <WiFi.h>
#include <SPIFFS.h>

#include <CAN.h>


void writeFile(const char * path, String message);
String readFile(const char * path);
void printFile();
void flashingFile();


const char* ssid = "NGUYENDUCSAOLAU1.2G";
const char* password = "28247467";

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

WiFiServer server(80);



void setup() {
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
  if (!CAN.begin(500E3)) {
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
  delay(1000);

  WiFiClient client = server.available();

  if (client) 
  {        
      String client_input = "";
      uint8_t i = 0;
      if (client.available())
      {
          client_input += client.readString();
      }
      // Serial.println(client_input);
      
      if (client_input.indexOf("/upload") != -1)
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
          flashingFile();
        }
      }
      else
      {
        Serial.println("New Client request");
        client.println("HTTP/1.1 200 OK");
        client.println("Content-type:text/html");
        client.println();
        client.println(html_request_upload);
        client.println();
      }
  }


  if (Serial.available() > 0)
  {
    printFile();

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
}


void flashingFile()
{
    // Reset message
    uint8_t reset_message[] = {0x07, 0x11, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    CAN.beginPacket(0x719);
    CAN.write(reset_message, sizeof(reset_message));
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