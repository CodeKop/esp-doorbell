#ifndef DOORBELL_RTSP_H
#define DOORBELL_RTSP_H

#include <Arduino.h>
#include "esp_camera.h"
#include <WiFi.h>
#include <ESP32-RTSPServer.h>
#include "env.h"
#include "camera_pins.h"

#ifdef HAVE_AUDIO
#include <ESP_I2S.h>
#endif

class DoorbellRTSP
{
public:
    // RTSPServer instance
    RTSPServer rtspServer;

    // Can set a username and password for RTSP authentication or leave blank for no authentication
    const char *rtspUser = "";
    const char *rtspPassword = "";

    // Variable to hold quality for RTSP frame
    int quality;
    // Task handles
    TaskHandle_t videoTaskHandle = NULL;
    TaskHandle_t audioTaskHandle = NULL;
    TaskHandle_t subtitlesTaskHandle = NULL;

    DoorbellRTSP();
    // ~DoorbellRTSP();
    bool init();
    bool init(RTSPServer::TransportType transport, uint16_t rtspPort, uint32_t sampleRate);
    // void deinit();
    // void reinit();
    // int rtspPort;
    // int sampleRate;
    // int transport;
    // int rtpVideoPort;
    // int rtpAudioPort;
    // int rtpSubtitlesPort;
    // IPAddress rtpIp;
    // int rtpTTL;

    void setup();
    void setupCamera();
    void getFrameQuality();

    /**
     * @brief Task to send jpeg frames via RTP.
     */
    void sendVideo(void *pvParameters);

    /**
     * @brief Task to send subtitles via RTP.
     */
    void sendSubtitles(void *pvParameters);

    // Timer callback function
    void onSubtitles(void *arg);

#ifdef HAVE_AUDIO
        // I2SClass object for I2S communication
        I2SClass I2S;

// I2S pins configuration
#define I2S_SCK 4 // Serial Clock (SCK) or Bit Clock (BCLK)
#define I2S_WS 5  // Word Select (WS) or Left Right Clock (LRCLK)
#define I2S_SDI 6 // Serial Data In (Mic)

    // Audio variables
    int sampleRate = 16000;          // Sample rate in Hz
    const size_t sampleBytes = 1024; // Sample buffer size (in bytes)
    int16_t *sampleBuffer = NULL;    // Pointer to the sample buffer

    /**
     * @brief Sets up the I2S microphone.
     *
     * @return true if setup is successful, false otherwise.
     */
    static bool setupMic();

    /**
     * @brief Reads audio data from the I2S microphone.
     *
     * @return The number of bytes read.
     */
    static size_t micInput();
#endif
};

#endif