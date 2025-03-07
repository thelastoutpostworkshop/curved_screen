// Tutorial : https://youtu.be/d49A0miFdqo

#ifndef _CONFIGURE_H
#define _CONFIGURE_H

// Comment the next line to compile and upload the code to the ESP32-S3 acting as the slaves, you can have as many slaves as you want
// Uncomment the next line to compile and upload the code to the ESP32-S3 acting as the master - only one master is allowed
// #define MASTER  

#define masterPort 80
#define MASTER_SERVERNAME "curved" // The name of the master web server, slaves are uting it to send calibration data
#define MAXRETRY 20         // Maximum number of retries for an HTTP call
#define PAUSEDELAYRETRY 1000 // Delay in ms before retrying a failed HTTP call

#define SLAVECOUNT 3 // The number of ESP32-S3 slaves

// Define the GPIO pin for the built-in RGB LED
#ifndef RGB_BUILTIN
#define RGB_BUILTIN 48 // Replace 48 with the actual pin number if different
#endif

#define PIN_SYNC_SHOW_FRAME 38 // Pin to used for sync signal to trigger slaves for showing a frame
#define SAFETY_WAIT_TIME_FRAME 5    // Value (in ms) added to the final calibration time of a frame for safety

#define MAX_FRAMES 256   // Maximum frames that the GIF can have, used for calibration, you can increase this value if needed
#define colorOutputSize 2 // 16 bit color as output
#define imageWidth 240
#define imageHeight 240

// Gif processing server

const String apiEndpoint = "http://192.168.1.90:8080/api/"; // The end point API for the GIF server, change this according to your local network
const String apiGif = "gif/";            // API name to get the GIF data


// Error codes
enum ErrorCode
{
    noError,
    noFrames,
    cannotGetGifFiles,
    cannotOpenGifFile,
    notEnoughMemory,
    cannotSendCalibrationValues,
    noMDNS,
    cannotGetIfMasterIsReady
};

#endif