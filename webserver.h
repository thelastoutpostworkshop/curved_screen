#ifndef _CURVEDSCREEN_WEB_SERVER_H
#define _CURVEDSCREEN_WEB_SERVER_H

uint8_t *getGifData(String, int, size_t *);
ErrorCode initWebServer();
ErrorCode sendCalibrationValues(String);
bool isMasterReady();

#endif