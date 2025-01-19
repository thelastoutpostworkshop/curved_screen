#ifndef _CURVEDSCREEN_WEB_SERVER_H
#define _CURVEDSCREEN_WEB_SERVER_H

const String apiEndpoint = "http://192.168.1.90/api/"; // The end point API for the GIF server, change this according to your local network
const String apiFrameCount = "frames-count";           // API name to get the frames count
const String apiGif = "gif/";                          // API name to get the GIF data

uint8_t *getGifData(String, int, size_t *);
ErrorCode initWebServer();
ErrorCode sendCalibrationValues(String);

#endif