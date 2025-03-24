#include <WiFi.h>
#include <ESP32-RTSPServer.h>
#include "esp_camera.h"
#include "DoorbellRTSP.h"
#include "env.h"
#include "camera_pins.h"

DoorbellRTSP rtspServer;

void printDeviceInfo()
{
  // Local function to format size
  auto fmtSize = [](size_t bytes) -> String
  {
    const char *sizes[] = {"B", "KB", "MB", "GB"};
    int order = 0;
    while (bytes >= 1024 && order < 3)
    {
      order++;
      bytes = bytes / 1024;
    }
    return String(bytes) + " " + sizes[order];
  };

  // Print device information
  Serial.println("");
  Serial.println("==== Device Information ====");
  Serial.printf("ESP32 Chip ID: %u\n", ESP.getEfuseMac());
  Serial.printf("Flash Chip Size: %s\n", fmtSize(ESP.getFlashChipSize()));
  if (psramFound())
  {
    Serial.printf("PSRAM Size: %s\n", fmtSize(ESP.getPsramSize()));
  }
  else
  {
    Serial.println("No PSRAM is found");
  }
  Serial.println("");
  // Print sketch information
  Serial.println("==== Sketch Information ====");
  Serial.printf("Sketch Size: %s\n", fmtSize(ESP.getSketchSize()));
  Serial.printf("Free Sketch Space: %s\n", fmtSize(ESP.getFreeSketchSpace()));
  Serial.printf("Sketch MD5: %s\n", ESP.getSketchMD5().c_str());
  Serial.println("");
  // Print task information
  Serial.println("==== Task Information ====");
  Serial.printf("Total tasks: %u\n", uxTaskGetNumberOfTasks() - 1);
  Serial.println("");
  // Print network information
  Serial.println("==== Network Information ====");
  Serial.printf("IP Address: %s\n", WiFi.localIP().toString().c_str());
  Serial.printf("MAC Address: %s\n", WiFi.macAddress().c_str());
  Serial.printf("SSID: %s\n", WiFi.SSID().c_str());
  Serial.printf("RSSI: %d dBm\n", WiFi.RSSI());
  Serial.println("");
  // Print RTSP server information
  Serial.println("==== RTSP Server Information ====");
  Serial.printf("RTSP Port: %d\n", rtspServer.rtspServer.rtspPort);
  Serial.printf("Sample Rate: %d\n", rtspServer.rtspServer.sampleRate);
  Serial.printf("Transport Type: %d\n", rtspServer.rtspServer.transport);
  Serial.printf("Video Port: %d\n", rtspServer.rtspServer.rtpVideoPort);
  Serial.printf("Audio Port: %d\n", rtspServer.rtspServer.rtpAudioPort);
  Serial.printf("Subtitles Port: %d\n", rtspServer.rtspServer.rtpSubtitlesPort);
  Serial.printf("RTP IP: %s\n", rtspServer.rtspServer.rtpIp.toString().c_str());
  Serial.printf("RTP TTL: %d\n", rtspServer.rtspServer.rtpTTL);
  Serial.println("");
  Serial.printf("RTSP Address: rtsp://%s:%d\n", WiFi.localIP().toString().c_str(), rtspServer.rtspServer.rtspPort);
  Serial.println("==============================");
  Serial.println("");
}

void sendVideoRTSP(void *pvParameters)
{
  rtspServer.sendVideo(pvParameters);
}

void onSubtitlesRTSP(void *arg)
{
  rtspServer.onSubtitles(arg);
}

void RTSPsetup()
{
  rtspServer.setupCamera();
  rtspServer.getFrameQuality();

#ifdef HAVE_AUDIO
  // Setup microphone
  if (rtspServer.setupMic())
  {
    Serial.println("Microphone Setup Complete");
    // Create tasks for sending audio
    // xTaskCreate(sendAudioRTSP, "Audio", 8192, NULL, 8, &audioTaskHandle);
  }
  else
  {
    Serial.println("Mic Setup Failed!");
  }
#endif

  // Create tasks for sending video, and subtitles
  xTaskCreate(sendVideoRTSP, "Video", 8192, NULL, 9, &this->videoTaskHandle);

  rtspServer.rtspServer.startSubtitlesTimer(onSubtitlesRTSP); // 1-second period

  rtspServer.rtspServer.maxRTSPClients = 5; // Set the maximum number of RTSP Multicast clients else enable OVERRIDE_RTSP_SINGLE_CLIENT_MODE to allow multiple clients for all transports eg. TCP, UDP, Multicast

  rtspServer.rtspServer.setCredentials(this->rtspUser, this->rtspPassword); // Set RTSP authentication

  // Initialize the RTSP server
#ifdef HAVE_AUDIO
  if (rtspServer.rtspServer.init(RTSPServer::VIDEO_AUDIO_SUBTITLES, 554, rtspServer.sampleRate))
  {
    Serial.printf("RTSP server started successfully, Connect to rtsp://%s:554/\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("Failed to start RTSP server");
  }
#else
  if (rtspServer.rtspServer.init())
  {
    Serial.printf("RTSP server started successfully using default values, Connect to rtsp://%s:554/\n", WiFi.localIP().toString().c_str());
  }
  else
  {
    Serial.println("Failed to start RTSP server");
  }
#endif
}

void setup()
{
  // Initialize serial communication
  Serial.begin(115200);

  // Connect to WiFi
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");

  RTSPSetup();
}

void loop()
{
  printDeviceInfo(); // just print out info about device
  delay(1000);
  vTaskDelete(NULL); // free 8k ram and delete the loop
}
