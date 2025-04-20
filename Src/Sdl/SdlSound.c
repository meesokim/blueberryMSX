/*****************************************************************************
** $Source: /cygdrive/d/Private/_SVNROOT/bluemsx/blueMSX/Src/Sdl/SdlSound.c,v $
**
** $Revision: 1.6 $
**
** $Date: 2008-03-30 18:38:45 $
**
** More info: http://www.bluemsx.com
**
** Copyright (C) 2003-2006 Daniel Vik
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
** 
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
******************************************************************************
*/
#include "ArchSound.h"
#include <SDL.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <stdbool.h>

int captureTimerThread(void* data);
// WAV 파일 헤더 구조체 추가 - 바이트 정렬 추가
#pragma pack(push, 1)  // 1바이트 정렬 시작
typedef struct {
    char riff[4];           // "RIFF"
    Uint32 fileSize;        // 파일 크기 - 8
    char wave[4];           // "WAVE"
    char fmt[4];            // "fmt "
    Uint32 fmtSize;         // 포맷 청크 크기 (16)
    Uint16 audioFormat;     // 오디오 포맷 (1 = PCM)
    Uint16 numChannels;     // 채널 수
    Uint32 sampleRate;      // 샘플 레이트
    Uint32 byteRate;        // 초당 바이트 수
    Uint16 blockAlign;      // 블록 정렬
    Uint16 bitsPerSample;   // 샘플당 비트 수
    char data[4];           // "data"
    Uint32 dataSize;        // 데이터 크기
} WavHeader;
#pragma pack(pop)  // 정렬 설정 복원

typedef struct SdlSound {
    Mixer* mixer;
    int started;
    UInt32 readPtr;
    UInt32 writePtr;
    UInt32 bytesPerSample;
    UInt32 bufferMask;
    UInt32 bufferSize;
    UInt32 skipCount;
    UInt8* buffer;
    SDL_AudioDeviceID deviceId;
    
    // WAV 캡처 관련 필드 추가
    FILE* wavFile;
    int captureEnabled;
    UInt32 capturedBytes;
    UInt32 sampleRate;
    UInt8 channels;
} SdlSound;


void printStatus(SDL_AudioDeviceID dev)
{
    switch (SDL_GetAudioDeviceStatus(dev))
    {
        case SDL_AUDIO_STOPPED: printf("stopped\n"); break;
        case SDL_AUDIO_PLAYING: printf("playing\n"); break;
        case SDL_AUDIO_PAUSED: printf("paused\n"); break;
        default: printf("???"); break;
    }
}

// 전역 변수 제거하고 구조체에 포함
SdlSound sdlSound;
int oldLen = 0;

// WAV 파일 헤더 작성 함수
void writeWavHeader(FILE* file, UInt32 sampleRate, UInt8 channels, UInt32 dataSize) {
    WavHeader header;
    memset(&header, 0, sizeof(WavHeader));
    
    // RIFF 헤더
    memcpy(header.riff, "RIFF", 4);
    header.fileSize = dataSize + 36;  // 데이터 크기 + 헤더 크기(44) - 8
    memcpy(header.wave, "WAVE", 4);
    
    // fmt 청크
    memcpy(header.fmt, "fmt ", 4);
    header.fmtSize = 16;
    header.audioFormat = 1; // PCM
    header.numChannels = channels;
    header.sampleRate = sampleRate;
    header.bitsPerSample = 16; // 16비트 샘플
    header.byteRate = sampleRate * channels * (header.bitsPerSample / 8);
    header.blockAlign = channels * (header.bitsPerSample / 8);
    
    // data 청크
    memcpy(header.data, "data", 4);
    header.dataSize = dataSize;
    
    // 파일 위치를 시작으로 이동
    fseek(file, 0, SEEK_SET);
    
    // 파일에 헤더 쓰기
    size_t written = fwrite(&header, sizeof(WavHeader), 1, file);
    if (written != 1) {
        printf("WAV header write error: %s\n", strerror(errno));
    }
    
    printf("WAV header channels=%d, samplerate=%d, bitsPerSample=%d\n", 
           header.numChannels, header.sampleRate, header.bitsPerSample);
}

// WAV 캡처 시작 함수
void startWavCapture(bool autoCapture) {
    if (autoCapture) {
        SDL_Thread* captureThread = SDL_CreateThread(captureTimerThread, "CaptureTimer", (void*)(intptr_t)30000);
        if (captureThread) {
            SDL_DetachThread(captureThread);
        } else {
            printf("Failed to create capture timer thread: %s\n", SDL_GetError());
        }
    }
    
    if (sdlSound.wavFile != NULL) {
        fclose(sdlSound.wavFile);
        sdlSound.wavFile = NULL;
    }
    
    // 현재 시간을 파일명에 포함
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char filename[256];
    sprintf(filename, "audio_capture_%04d%02d%02d_%02d%02d%02d.wav", 
            t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
            t->tm_hour, t->tm_min, t->tm_sec);
    
    sdlSound.wavFile = fopen(filename, "wb");
    if (sdlSound.wavFile == NULL) {
        fprintf(stderr, "Failed to create WAV capture file: %s\n", filename);
        return;
    }
    
    // 빈 헤더 작성 (나중에 업데이트)
    writeWavHeader(sdlSound.wavFile, sdlSound.sampleRate, sdlSound.channels, 0);
    
    // 캡처 활성화 플래그를 1로 설정 (0에서 1로 변경)
    sdlSound.captureEnabled = 1;
    sdlSound.capturedBytes = 0;
    
    printf("Audio capture started: %s (captureEnabled=%d)\n", filename, sdlSound.captureEnabled);
}

// WAV 캡처 종료 함수
void stopWavCapture() {
    if (sdlSound.wavFile != NULL && sdlSound.captureEnabled) {
        // 파일 크기 확인
        long fileSize = ftell(sdlSound.wavFile);
        
        // 파일 시작으로 되돌아가서 헤더 업데이트
        fseek(sdlSound.wavFile, 0, SEEK_SET);
        
        // 실제 데이터 크기 계산 (헤더 크기 제외)
        UInt32 dataSize = (fileSize > 44) ? (fileSize - 44) : 0;
        
        // 헤더 다시 쓰기
        writeWavHeader(sdlSound.wavFile, sdlSound.sampleRate, sdlSound.channels, dataSize);
        
        printf("WAV 파일 헤더 업데이트: 데이터 크기 = %u 바이트\n", dataSize);
        
        fclose(sdlSound.wavFile);
        sdlSound.wavFile = NULL;
        sdlSound.captureEnabled = 0;
        
        printf("Audio capture stopped. Captured %u bytes.\n", sdlSound.capturedBytes);
    }
}

// 캡처 타이머 스레드 함수 추가
int captureTimerThread(void* data) {
    int duration = (int)(intptr_t)data;
    printf("Audio capture will run for %d seconds\n", duration/1000);
    SDL_Delay(duration);
    printf("Capture timer expired, stopping capture...\n");
    stopWavCapture();
    return 0;
}

void soundCallback(void* userdata, Uint8* stream, int length)
{
    SdlSound* sound = (SdlSound*)userdata;
    UInt32 avail = (sound->writePtr - sound->readPtr) & sound->bufferMask;
    oldLen = length;

    // 디버깅 정보 추가
    // if (length > 0) {
    //     static int callCount = 0;
    //     if (callCount++ % 1000 == 0) {
    //         printf("Audio callback: length=%d, avail=%d, writePtr=%d, readPtr=%d\n", 
    //                length, avail, sound->writePtr, sound->readPtr);
    //     }
    // }

    // 버퍼에 데이터가 있는지 확인
    if (length > avail) {
        // 버퍼 언더런 처리 - 이전 프레임 데이터 유지하여 끊김 방지
        if (sound->readPtr > 0 && sound->readPtr < sound->bufferSize) {
            // 이전 데이터를 재사용하여 부드럽게 처리
            UInt32 prevDataSize = (sound->readPtr > length) ? length : sound->readPtr;
            memcpy(stream, sound->buffer + sound->readPtr - prevDataSize, prevDataSize);
            
            if (length > prevDataSize) {
                // 나머지 부분은 0으로 채움
                memset(stream + prevDataSize, 0, length - prevDataSize);
            }
        } else {
            memset(stream, 0, length);
        }
        
        // 캡처 중이면 무음 데이터도 캡처
        if (sound->captureEnabled && sound->wavFile != NULL) {
            fwrite(stream, 1, length, sound->wavFile);
            sound->capturedBytes += length;
        }
        return;
    }

    // 버퍼에서 데이터 읽기 - 보간 추가하여 부드럽게 처리
    if (sound->readPtr + length > sound->bufferSize) {
        UInt32 len1 = sound->bufferSize - sound->readPtr;
        UInt32 len2 = length - len1;
        memcpy(stream, sound->buffer + sound->readPtr, len1);
        memcpy(stream + len1, sound->buffer, len2);
        sound->readPtr = len2;
    } 
    else 
    {
        memcpy(stream, sound->buffer + sound->readPtr, length);
        sound->readPtr = (sound->readPtr + length) & sound->bufferMask;
    }
    
    // 캡처 중이면 오디오 데이터 캡처
    if (sound->captureEnabled && sound->wavFile != NULL) {
        fwrite(stream, 1, length, sound->wavFile);
        sound->capturedBytes += length;
    }
}

static Int32 soundWrite(void* dummy, Int16 *buffer, UInt32 count)
{
    UInt32 avail;
    Mixer* mixer = sdlSound.mixer;

    if (!sdlSound.started || mixer == NULL) {
        return 0;
    }
    
    // 디버깅 정보 추가 - 주석 해제
    static int writeCount = 0;
    if (writeCount++ % 1000 == 0) {
        printf("soundWrite called: count=%d bytes, buffer[0]=%d, captureEnabled=%d, channels=%d\n", 
               count * sdlSound.bytesPerSample, buffer[0], sdlSound.captureEnabled, sdlSound.channels);
    }

    // 직접 WAV 파일에 쓰기 (믹서 출력 캡처)
    if (sdlSound.captureEnabled && sdlSound.wavFile != NULL) {
        // 데이터 검증
        int isAllZero = 1;
        for (int i = 0; i < count; i++) {
            if (buffer[i] != 0) {
                isAllZero = 0;
                break;
            }
        }
        
        if (!isAllZero) {
            static int directWriteCount = 0;
            if (directWriteCount++ < 10) { // 처음 10번만 로그 출력
                printf("Direct capture: %d samples, first sample=%d, channels=%d\n", 
                       count, buffer[0], sdlSound.channels);
            }
            
            // 원본 오디오 데이터를 WAV 파일에 직접 기록
            // 채널 수에 맞게 데이터 쓰기
            size_t written;
            if (sdlSound.channels == 2) {
                // 스테레오 데이터 그대로 쓰기
                written = fwrite(buffer, 2 * sizeof(Int16), count / 2, sdlSound.wavFile);
                if (written != count / 2) {
                    printf("WAV 스테레오 쓰기 실패: %s\n", strerror(errno));
                }
                sdlSound.capturedBytes += count * sizeof(Int16);
            } else {
                // 모노 데이터 쓰기
                written = fwrite(buffer, sizeof(Int16), count, sdlSound.wavFile);
                if (written != count) {
                    printf("WAV 모노 쓰기 실패: %s\n", strerror(errno));
                }
                sdlSound.capturedBytes += count * sizeof(Int16);
            }
            
            // 파일 버퍼 즉시 디스크에 쓰기
            fflush(sdlSound.wavFile);
        }
    }

    count *= sdlSound.bytesPerSample/2;

    // if (sdlSound.skipCount > 0) {
    //     if (count <= sdlSound.skipCount) {
    //         sdlSound.skipCount -= count;
    //         return 0;
    //     }
    //     count -= sdlSound.skipCount;
    //     sdlSound.skipCount = 0;
    // }

    SDL_LockAudioDevice(sdlSound.deviceId);

    avail = (sdlSound.readPtr - sdlSound.writePtr - 1) & sdlSound.bufferMask;
    if (count > avail) {
        // buffer overrun
        SDL_UnlockAudioDevice(sdlSound.deviceId);
        return 0;
    }

    if (sdlSound.writePtr + count > sdlSound.bufferSize) {
        UInt32 count1 = sdlSound.bufferSize - sdlSound.writePtr;
        UInt32 count2 = count - count1;
        memcpy(sdlSound.buffer + sdlSound.writePtr, buffer, count1);
        memcpy(sdlSound.buffer, (UInt8*)buffer + count1, count2);
        sdlSound.writePtr = count2;
    }
    else {
        memcpy(sdlSound.buffer + sdlSound.writePtr, buffer, count);
        sdlSound.writePtr = (sdlSound.writePtr + count) & sdlSound.bufferMask;
    }

    SDL_UnlockAudioDevice(sdlSound.deviceId);
    return 0;
}

void archSoundCreate(Mixer* mixer, UInt32 sampleRate, UInt32 bufferSize, Int16 channels) 
{
    SDL_AudioSpec desired;
    SDL_AudioSpec obtained;
    UInt16 samples = channels;

    memset(&sdlSound, 0, sizeof(sdlSound));

    bufferSize = bufferSize * sampleRate / 1000 * sizeof(Int16) / 4;

    while (samples < bufferSize) samples *= 2;

    // SDL Audio Subsystem Initialization
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        fprintf(stderr, "Failed to initialize SDL audio: %s\n", SDL_GetError());
        return;
    }

    // Audio Device Initialization
    desired.freq = sampleRate;
    desired.samples = samples;
    desired.channels = (UInt8)channels;
#ifdef LSB_FIRST
    desired.format = AUDIO_S16LSB;
#else
    desired.format = AUDIO_S16MSB;
#endif
    desired.callback = soundCallback;
    desired.userdata = &sdlSound;

    // Available audio devices ouput
    int numDevices = SDL_GetNumAudioDevices(0);
    printf("Available audio devices:\n");
    for (int i = 0; i < numDevices; i++) {
        printf("  %d: %s\n", i, SDL_GetAudioDeviceName(i, 0));
    }

    // Default audio device
    sdlSound.deviceId = SDL_OpenAudioDevice(NULL, 0, &desired, &obtained, 0);
    if (sdlSound.deviceId == 0) {
        fprintf(stderr, "Failed to open audio device: %s\n", SDL_GetError());
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }

    printf("Audio device opened successfully (ID: %u)\n", sdlSound.deviceId);
    printf("Obtained audio specs:\n");
    printf("  Frequency: %d Hz\n", obtained.freq);
    printf("  Format: %d\n", obtained.format);
    printf("  Channels: %d\n", obtained.channels);
    printf("  Samples: %d\n", obtained.samples);
    printf("  Size: %d bytes\n", obtained.size);

    // buffer setting
    sdlSound.bufferSize = obtained.size * 4;
    printf("Buffer size: %d bytes\n", sdlSound.bufferSize);
    
    // reisze buffer size
    UInt32 powerOfTwo = 1;
    while (powerOfTwo < sdlSound.bufferSize) {
        powerOfTwo *= 2;
    }
    sdlSound.bufferSize = powerOfTwo;
    sdlSound.bufferMask = sdlSound.bufferSize - 1;
    sdlSound.buffer = (UInt8*)calloc(1, sdlSound.bufferSize);
    
    if (sdlSound.buffer == NULL) {
        fprintf(stderr, "Failed to allocate audio buffer\n");
        SDL_CloseAudioDevice(sdlSound.deviceId);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        return;
    }

    sdlSound.started = 1;
    sdlSound.mixer = mixer;
    sdlSound.bytesPerSample = (obtained.format == AUDIO_U8 || obtained.format == AUDIO_S8) ? 1 : 2;
    sdlSound.bytesPerSample *= obtained.channels;
    
    sdlSound.sampleRate = obtained.freq;
    sdlSound.channels = obtained.channels;

    mixerSetStereo(mixer, obtained.channels == 2);
    
    for (int i = 0; i < MIXER_CHANNEL_TYPE_COUNT; i++) {
        mixerSetChannelTypeVolume(mixer, i, 100);
        mixerEnableChannelType(mixer, i, 1);
    }
    
    mixerSetWriteCallback(mixer, soundWrite, NULL, obtained.samples * 2);
    
    printf("Mixer callback set: buffer size = %d samples\n", obtained.samples * 2);

    Int16 tempBuffer[obtained.samples * 2];
    memset(tempBuffer, 0, sizeof(tempBuffer));
    soundWrite(NULL, tempBuffer, obtained.samples);
    
    SDL_PauseAudioDevice(sdlSound.deviceId, 0);
    printf("Audio device status: ");
    printStatus(sdlSound.deviceId);
    
    startWavCapture(true);
}

void archSoundDestroy(void) 
{
    stopWavCapture();
    
    if (sdlSound.started) {
        mixerSetWriteCallback(sdlSound.mixer, NULL, NULL, 0);
        SDL_CloseAudioDevice(sdlSound.deviceId);
        SDL_QuitSubSystem(SDL_INIT_AUDIO);
        free(sdlSound.buffer);
        sdlSound.buffer = NULL;
    }
    sdlSound.started = 0;
}

void archSoundResume(void) 
{
    if (sdlSound.started) {
        SDL_PauseAudioDevice(sdlSound.deviceId, 0);
    }
}

void archSoundSuspend(void) 
{
    if (sdlSound.started) {
        SDL_PauseAudioDevice(sdlSound.deviceId, 1);
    }
}

// WAV 캡처 수동 제어 함수 추가
void archSoundStartCapture(void) {
    startWavCapture(false);
}

void archSoundStopCapture(void) {
    stopWavCapture();
}

void archSoundSync(UInt32 framesToGenerate) {
    if (sdlSound.started && sdlSound.mixer != NULL) {
        mixerSync(sdlSound.mixer);
    }  
    //     Int16 tempBuffer[1024];
    //     if (mixerRead(sdlSound.mixer, tempBuffer, 1024 / sdlSound.channels)) {
    //         // 생성된 데이터가 있으면 버퍼에 추가
    //         SDL_LockAudioDevice(sdlSound.deviceId);
    //         UInt32 count = 1024 * sizeof(Int16);
    //         UInt32 avail = (sdlSound.readPtr - sdlSound.writePtr - 1) & sdlSound.bufferMask;
            
    //         if (count <= avail) {
    //             if (sdlSound.writePtr + count > sdlSound.bufferSize) {
    //                 UInt32 count1 = sdlSound.bufferSize - sdlSound.writePtr;
    //                 UInt32 count2 = count - count1;
    //                 memcpy(sdlSound.buffer + sdlSound.writePtr, tempBuffer, count1);
    //                 memcpy(sdlSound.buffer, (UInt8*)tempBuffer + count1, count2);
    //                 sdlSound.writePtr = count2;
    //             }
    //             else {
    //                 memcpy(sdlSound.buffer + sdlSound.writePtr, tempBuffer, count);
    //                 sdlSound.writePtr = (sdlSound.writePtr + count) & sdlSound.bufferMask;
    //             }
                
    //             static int syncCount = 0;
    //             if (syncCount++ % 100 == 0) {
    //                 printf("Manual buffer fill: %d bytes, writePtr=%d, readPtr=%d\n", 
    //                        count, sdlSound.writePtr, sdlSound.readPtr);
    //             }
    //         }
    //         SDL_UnlockAudioDevice(sdlSound.deviceId);
    //     }
    // }
}
