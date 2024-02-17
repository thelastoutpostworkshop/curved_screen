#include <ESPAsyncWebServer.h>
#include <ESPmDNS.h>
#include "secrets.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");


class Frames
{
public:
    int frameCount = 0;
    size_t *dataSizes = nullptr;   // Array to store the size of each frame
    uint8_t **frameData = nullptr; // Pointer to an array of pointers to store frame data

    Frames() : frameData(nullptr), frameCount(0), dataSizes(nullptr) {}

    ~Frames()
    {
        // Free each frame
        for (int i = 0; i < frameCount; i++)
        {
            delete[] frameData[i];
        }
        // Free the array of pointers
        delete[] frameData;
        // Free the array of sizes
        delete[] dataSizes;
    }

    void addFrame(const uint8_t *data, size_t size)
    {
        // Allocate new array of pointers with an extra slot for the new frame
        uint8_t **newFrameData = new uint8_t *[frameCount + 1];
        size_t *newDataSizes = new size_t[frameCount + 1];

        // Copy existing frame pointers and sizes to the new array
        for (int i = 0; i < frameCount; i++)
        {
            newFrameData[i] = frameData[i];
            newDataSizes[i] = dataSizes[i];
        }

        // Allocate memory for the new frame and copy its data
        newFrameData[frameCount] = new uint8_t[size];
        memcpy(newFrameData[frameCount], data, size);
        newDataSizes[frameCount] = size;

        // Free old arrays
        delete[] frameData;
        delete[] dataSizes;

        // Update class members to point to the new arrays
        frameData = newFrameData;
        dataSizes = newDataSizes;
        frameCount++;
    }
};

class Images {
public:
    int imagesCount = 0;
    Frames* framesArray = nullptr; // Pointer to an array of Frames objects

    Images() : framesArray(nullptr), imagesCount(0) {}

    ~Images() {
        // Free the dynamically allocated Frames objects
        delete[] framesArray;
    }

    void addFrames(const Frames& newFrames) {
        // Create a new array with one more slot than the current array
        Frames* newFramesArray = new Frames[imagesCount + 1];
        
        // Copy existing Frames objects to the new array
        for (int i = 0; i < imagesCount; i++) {
            newFramesArray[i] = framesArray[i];
        }

        // Add the new Frames object to the new array
        newFramesArray[imagesCount] = newFrames;

        // Free the old array
        delete[] framesArray;

        // Update the pointer and the count
        framesArray = newFramesArray;
        imagesCount++;
    }
};


class ESP32Server
{

private:
    Frames myFrames;

    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
    {
        Serial.printf("Data received: %lu bytes\n", len);
    }

    void imageReceive(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                      void *arg, uint8_t *data, size_t len)
    {
        if (type == WS_EVT_DATA)
        {
            handleWebSocketMessage(arg, data, len);
        }
        // Handle other events like WS_EVT_CONNECT, WS_EVT_DISCONNECT, etc.
    }

    static void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type,
                                 void *arg, uint8_t *data, size_t len)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;

        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            // Add any additional actions you want to perform when the WebSocket is opened
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            // Handle disconnection
            break;
        case WS_EVT_DATA:
            if (info->final && info->index == 0 && info->len == len)
            {
                // The whole message is in a single frame and we got all of it's data
                if (info->opcode == WS_TEXT)
                {
                    // Data is text
                    Serial.println("Text data received");
                    // Convert the data to a string to process it as text
                    String message = "";
                    for (size_t i = 0; i < len; i++)
                    {
                        message += (char)data[i];
                    }
                    Serial.println(message);
                }
                else if (info->opcode == WS_BINARY)
                {
                    // Data is binary
                    Serial.println("Binary data received");
                    // Process the binary data directly...
                }
            }
            else
            {
                Serial.println("Not final data");
            }
            break;
        case WS_EVT_PONG:
            break;
        case WS_EVT_ERROR:
            // Handle PONG (response to ping) and ERROR events
            break;
        }
    }

public:
    Images images;
    void initWebServer()
    {
        Serial.println("Connecting to WiFi");
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(300);
            Serial.print(".");
        }
        Serial.println("Connected");
        Serial.print("IP Address: ");
        Serial.println(WiFi.localIP());
        if (!MDNS.begin("sphere"))
        {
            Serial.println("Error starting mDNS");
            return;
        }

        ws.onEvent(onWebSocketEvent);
        server.addHandler(&ws);

        server.begin();
    }
};
