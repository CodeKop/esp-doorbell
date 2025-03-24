#include "DoorbellRTSP.h"

DoorbellRTSP::DoorbellRTSP() {}

bool DoorbellRTSP::init()
{
    this->rtspServer.init();
}

bool DoorbellRTSP::init(RTSPServer::TransportType transport, uint16_t rtspPort, uint32_t sampleRate)
{
    this->rtspServer.init(transport, rtspPort, sampleRate);
}

/**
 * @brief Sets up the camera with the specified configuration.
 */
void DoorbellRTSP::setupCamera()
{
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer = LEDC_TIMER_0;
    config.pin_d0 = Y2_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.frame_size = FRAMESIZE_UXGA;
    config.pixel_format = PIXFORMAT_JPEG; // for streaming
    config.grab_mode = CAMERA_GRAB_LATEST;
    config.fb_location = CAMERA_FB_IN_PSRAM;
    config.jpeg_quality = 10;
    config.fb_count = 2;

    // if PSRAM IC present, init with UXGA resolution and higher JPEG quality
    // for larger pre-allocated frame buffer.
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        if (psramFound())
        {
            config.jpeg_quality = 10;
            config.fb_count = 2;
            config.grab_mode = CAMERA_GRAB_LATEST;
        }
        else
        {
            // Limit the frame size when PSRAM is not available
            config.frame_size = FRAMESIZE_SVGA;
            config.fb_location = CAMERA_FB_IN_DRAM;
        }
    }
    else
    {
        // Best option for face detection/recognition
        config.frame_size = FRAMESIZE_240X240;
#if CONFIG_IDF_TARGET_ESP32S3
        config.fb_count = 2;
#endif
    }

#if defined(CAMERA_MODEL_ESP_EYE)
    pinMode(13, INPUT_PULLUP);
    pinMode(14, INPUT_PULLUP);
#endif

    // Initialize camera
    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK)
    {
        Serial.printf("Camera init failed with error 0x%x", err);
        return;
    }

    sensor_t *s = esp_camera_sensor_get();
    // initial sensors are flipped vertically and colors are a bit saturated
    if (s->id.PID == OV3660_PID)
    {
        s->set_vflip(s, 1);       // flip it back
        s->set_brightness(s, 1);  // up the brightness just a bit
        s->set_saturation(s, -2); // lower the saturation
    }
    // drop down frame size for higher initial frame rate
    if (config.pixel_format == PIXFORMAT_JPEG)
    {
        s->set_framesize(s, FRAMESIZE_QVGA);
    }

#if defined(CAMERA_MODEL_M5STACK_WIDE) || defined(CAMERA_MODEL_M5STACK_ESP32CAM)
    s->set_vflip(s, 1);
    s->set_hmirror(s, 1);
#endif

#if defined(CAMERA_MODEL_ESP32S3_EYE)
    s->set_vflip(s, 1);
#endif
    Serial.println("Camera Setup Complete");
}

/**
 * @brief Retrieves the current frame quality from the camera.
 */
void DoorbellRTSP::getFrameQuality()
{
    sensor_t *s = esp_camera_sensor_get();
    this->quality = s->status.quality;
    Serial.printf("Camera Quality is: %d\n", quality);
}

/**
 * @brief Task to send jpeg frames via RTP.
 */
void DoorbellRTSP::sendVideo(void *pvParameters)
{
    while (true)
    {
        // Send frame via RTP
        if (this->rtspServer.readyToSendFrame())
        {
            camera_fb_t *fb = esp_camera_fb_get();
            this->rtspServer.sendRTSPFrame(fb->buf, fb->len, this->quality, fb->width, fb->height);
            esp_camera_fb_return(fb);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}

/**
 * @brief Task to send subtitles via RTP.
 */
void DoorbellRTSP::sendSubtitles(void *pvParameters)
{
    char data[100];
    while (true)
    {
        if (this->rtspServer.readyToSendSubtitles())
        {
            size_t len = snprintf(data, sizeof(data), "FPS: %lu", this->rtspServer.rtpFps);
            this->rtspServer.sendRTSPSubtitles(data, len);
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS); // Delay for 1 second has to be 1 second
    }
}

// Timer callback function
void DoorbellRTSP::onSubtitles(void *arg)
{
    char data[100];
    if (this->rtspServer.readyToSendSubtitles())
    {
        size_t len = snprintf(data, sizeof(data), "FPS: %lu", this->rtspServer.rtpFps);
        this->rtspServer.sendRTSPSubtitles(data, len);
    }
}

#ifdef HAVE_AUDIO
/**
 * @brief Sets up the I2S microphone.
 *
 * @return true if setup is successful, false otherwise.
 */
static bool DoorbellRTSP::setupMic()
{
    bool res;
    // I2S mic and I2S amp can share same I2S channel
    this->I2S.setPins(I2S_SCK, I2S_WS, -1, I2S_SDI, -1); // BCLK/SCK, LRCLK/WS, SDOUT, SDIN, MCLK
    res = this->I2S.begin(I2S_MODE_STD, sampleRate, I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT);
    if (sampleBuffer == NULL)
        sampleBuffer = (int16_t *)malloc(sampleBytes);
    return res;
}

/**
 * @brief Reads audio data from the I2S microphone.
 *
 * @return The number of bytes read.
 */
static size_t DoorbellRTSP::micInput()
{
    // read esp mic
    size_t bytesRead = 0;
    bytesRead = this->I2S.readBytes((char *)sampleBuffer, sampleBytes);
    return bytesRead;
}

/**
 * @brief Task to send audio data via RTP.
 */
void DoorbellRTSP::sendAudio(void *pvParameters)
{
    while (true)
    {
        size_t bytesRead = 0;
        if (this->rtspServer.readyToSendAudio())
        {
            bytesRead = micInput();
            if (bytesRead)
                this->rtspServer.sendRTSPAudio(this->sampleBuffer, bytesRead);
            else
                Serial.println("No audio Recieved");
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // Delay for 1 second
    }
}
#endif
