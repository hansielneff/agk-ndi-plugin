#define WIN32_LEAN_AND_MEAN // Exclude rarely-used stuff from Windows headers
#include <windows.h>
#include <stdlib.h>
#include <stdio.h>

#include "../AGKLibraryCommands.h"

#include "Processing.NDI.Lib.h"

NDIlib_find_instance_t NDIFinder = nullptr;
NDIlib_recv_instance_t NDIReceiver = nullptr;
NDIlib_send_instance_t NDISender = nullptr;

uint32_t sourceCount = 0;
char** sourcesArray = nullptr;

extern "C" DLL_EXPORT int Initialize() {
    if (!NDIlib_initialize()) return 0;
    return 1;
}

extern "C" DLL_EXPORT void Uninitialize() {
    NDIlib_destroy();
}

extern "C" DLL_EXPORT int CreateFinder() {
    if (NDIFinder) return 0;
    if (!(NDIFinder = NDIlib_find_create_v2(nullptr))) return 0;
    return 1;
}

extern "C" DLL_EXPORT void DeleteFinder() {
    if (!NDIFinder) return;
    NDIlib_find_destroy(NDIFinder);
    NDIFinder = nullptr;
}

extern "C" DLL_EXPORT const char* FindSources(int timeout) {
    if (!NDIFinder) return "";

    // Wait specified timeout and then get currently discovered sources
    NDIlib_find_wait_for_sources(NDIFinder, timeout);
    const NDIlib_source_t* sources = NDIlib_find_get_current_sources(NDIFinder, &sourceCount);

    // Doing this once more seems to help
    if (NDIlib_find_wait_for_sources(NDIFinder, timeout))
        sources = NDIlib_find_get_current_sources(NDIFinder, &sourceCount);

    // Allocate string array for storing source names
    sourcesArray = (char**)malloc(sourceCount * sizeof(char*));
    if (!sourcesArray) return "";

    // Allocate memory for strings
    int totalStringBytes = 0;
    for (uint32_t i = 0; i < sourceCount; i++) {
        int stringBytes = strlen(sources[i].p_ndi_name) + 2; // +2 for newline and null-byte
        totalStringBytes += stringBytes;
        sourcesArray[i] = (char*)malloc(stringBytes);
        strcpy(sourcesArray[i], sources[i].p_ndi_name);
        sourcesArray[i][stringBytes - 2] = '\n';
        sourcesArray[i][stringBytes - 1] = '\0';
    }

    // Allocate AGK-controlled output string and concatenate everything into it
    char* outputString = agk::CreateString(totalStringBytes);
    outputString[0] = '\0';
    for (uint32_t i = 0; i < sourceCount; i++) strcat(outputString, sourcesArray[i]);

    // Remove newline so other functions can check which NDI names were discovered
    for (uint32_t i = 0; i < sourceCount; i++) sourcesArray[i][strlen(sourcesArray[i]) - 1] = '\0';

    return outputString;
}

extern "C" DLL_EXPORT int CreateReceiver() {
    if (NDIReceiver) return 0;
    NDIlib_recv_create_v3_t settings;
    settings.color_format = NDIlib_recv_color_format_RGBX_RGBA;
    if (!(NDIReceiver = NDIlib_recv_create_v3(&settings))) return 0;
    return 1;
}

extern "C" DLL_EXPORT void DeleteReceiver() {
    if (!NDIReceiver) return;
    NDIlib_recv_destroy(NDIReceiver);
    NDIReceiver = nullptr;
}

extern "C" DLL_EXPORT int ConnectReceiver(const char* senderName) {
    if (!NDIReceiver) return 0;

    // Only connect to senderName if it's the name of a discovered source
    for (uint32_t i = 0; i < sourceCount; i++) {
        if (!strcmp(sourcesArray[i], senderName)) {
            NDIlib_source_t source;
            source.p_ndi_name = senderName;
            NDIlib_recv_connect(NDIReceiver, &source);
            return 1;
        }
    }

    return 0;
}

extern "C" DLL_EXPORT void ReceiveFrame(int outputImageID, int outputSoundID, int timeout) {
    if (!NDIReceiver) return;

    NDIlib_video_frame_v2_t video;
    NDIlib_audio_frame_v3_t audio;

    switch (NDIlib_recv_capture_v3(NDIReceiver, &video, &audio, nullptr, timeout)) {
    case NDIlib_frame_type_video:
        // This plugin currently only accepts RGBA-format
        if (video.FourCC == NDIlib_FourCC_type_RGBA) {
            // Create memblock for AGK image (12 bytes for metadata)
            int dataOffset = 12;
            int dataSize = 4 * video.xres * video.yres;
            int memblock = agk::CreateMemblock(dataOffset + dataSize);
            agk::SetMemblockInt(memblock, 0, video.xres);
            agk::SetMemblockInt(memblock, 4, video.yres);
            agk::SetMemblockInt(memblock, 8, 32); // AGK only deals with 32-bit image data

            // Copy NDI image data to AGK image data
            uint8_t* imageData = agk::GetMemblockPtr(memblock) + dataOffset;
            memcpy(imageData, video.p_data, dataSize);

            // Create new AGK image from received image data
            agk::CreateImageFromMemblock(outputImageID, memblock);
            
            agk::DeleteMemblock(memblock);
        }
        else agk::Print("Incompatible data received");

        NDIlib_recv_free_video_v2(NDIReceiver, &video);
        break;
    case NDIlib_frame_type_audio:
        // Process audio here
        NDIlib_recv_free_audio_v3(NDIReceiver, &audio);
        break;
    case NDIlib_frame_type_error:
        agk::Print("Trying to receive data produced an unknown error");
        break;
    }

    agk::Sync();
}

extern "C" DLL_EXPORT int CreateSender() {
    if (NDISender) return 0;
    if (!(NDISender = NDIlib_send_create(nullptr))) return 0;
    return 1;
}

extern "C" DLL_EXPORT void DeleteSender() {
    if (!NDISender) return;
    NDIlib_send_destroy(NDISender);
    NDISender = nullptr;
}

extern "C" DLL_EXPORT void SendFrame(int imageID) {
    if (!NDISender) return;

    int imageWidth = (int) agk::GetImageWidth(imageID);
    int imageHeight = (int) agk::GetImageHeight(imageID);
    int memblock = agk::CreateMemblockFromImage(imageID);
    int dataOffset = 12;
    int dataSize = agk::GetMemblockSize(memblock) - dataOffset;

    // Create NDI frame and set p_data to point to the AGK image data
    NDIlib_video_frame_v2_t NDIFrame;
    NDIFrame.xres = imageWidth;
    NDIFrame.yres = imageHeight;
    NDIFrame.FourCC = NDIlib_FourCC_type_RGBA;
    NDIFrame.p_data = (uint8_t*)(agk::GetMemblockPtr(memblock) + dataOffset);

    NDIlib_send_send_video_v2(NDISender, &NDIFrame);
    agk::DeleteMemblock(memblock);
}
