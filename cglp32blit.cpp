#if defined _WIN32 || defined __CYGWIN__
    #define NOMINMAX 
    #include <windows.h>
    #undef TRANSPARENT
#endif
#if defined(TARGET_32BLIT_HW) || defined(PICO_BUILD)
#include <malloc.h>
#endif
#include <stdlib.h>nin
#include <float.h>
#include <32blit.hpp>
#include <string.h>
#include "machineDependent.h"
#include "cglp.h"
#include "cglp32blit.hpp"
#include <math.h>

using namespace blit;

#define TONEDIV 1

#define TONE_PER_NOTE (32 / TONEDIV)
#define SOUND_TONE_COUNT (64 / TONEDIV)
  

static int WINDOW_WIDTH = DEFAULT_WINDOW_WIDTH;
static int WINDOW_HEIGHT = DEFAULT_WINDOW_HEIGHT;
static float scale = 1.0f;
static int viewW = DEFAULT_WINDOW_WIDTH;
static int viewH = DEFAULT_WINDOW_HEIGHT;
static int origViewW = DEFAULT_WINDOW_WIDTH;
static int origViewH = DEFAULT_WINDOW_HEIGHT;
static unsigned char clearColorR = 0;
static unsigned char clearColorG = 0;
static unsigned char clearColorB = 0;
static int offsetX = 0;
static int offsetY = 0;
static float wscale = 1.0f;
static bool debugMode = true;
static float mouseX, mouseY;
static int channel = 0;

typedef struct {
    float freq;
    float duration;
    float when;
  } SoundTone;
  
static SoundTone soundTones[SOUND_TONE_COUNT];
static int soundToneIndex = 0;
static float soundTime = 0;

static uint32_t prevtime = 0;
static uint32_t starttime = 0;

extern SaveData saveData;

static void loadHighScores()
{
    read_save(saveData);
}

static void saveHighScores()
{
    write_save(saveData);
}

static void initSoundTones() 
{
    for (int i = 0; i < SOUND_TONE_COUNT; i++) 
    {
      soundTones[i].when = FLT_MAX;
    }
}
  
static void addSoundTone(float freq, float duration, float when) 
{
    SoundTone *st = &soundTones[soundToneIndex];
    st->freq = freq;
    st->duration = duration;
    st->when = when;
    soundToneIndex++;
    if (soundToneIndex >= SOUND_TONE_COUNT) 
    {
      soundToneIndex = 0;
    }
}
  

void md_playTone(float freq, float duration, float when)
{ 
    addSoundTone(freq, duration, when); 
}

static void updateFromSoundTask() {
    soundTime += (60.0f * 60.0f /50.0f) / tempo / (TONE_PER_NOTE * TONEDIV);
    float lastWhen = 0;
    int ti = -1;
    for (int i = 0; i < SOUND_TONE_COUNT; i++) {
        SoundTone *st = &soundTones[i];
        if (st->when <= soundTime) {
            if (st->when > lastWhen) {
                ti = i;
                lastWhen = st->when;
                st->when = FLT_MAX;
            }
        }        
    }
    if (ti >= 0) {
        SoundTone *st = &soundTones[ti];
        channels[channel].waveforms   = Waveform::SQUARE;
        channels[channel].attack_ms   = uint16_t(trunc(st->duration * 1000.0f * 60.0f / 50.0f));
        channels[channel].decay_ms    = 16;
        channels[channel].sustain     = 0;
        channels[channel].release_ms  = 0;
        channels[channel].volume      = 0xffff;
        channels[channel].frequency   = uint16_t(trunc(st->freq));
        channels[channel].trigger_attack();    
        channel++;
        if(channel == CHANNEL_COUNT)
            channel = 0;
    }
}

void md_stopTone()
{
    initSoundTones();
}

float md_getAudioTime()
{
    return soundTime;
}

void md_drawCharacter(unsigned char grid[CHARACTER_HEIGHT][CHARACTER_WIDTH][3],
                     float x, float y, int hash) 
{
    for (int yy = 0; yy < CHARACTER_HEIGHT; yy++) {
        for (int xx = 0; xx < CHARACTER_WIDTH; xx++) {
            unsigned char r = grid[yy][xx][0];
            unsigned char g = grid[yy][xx][1];
            unsigned char b = grid[yy][xx][2];
            
            if ((r == 0) && (g == 0) && (b == 0)) continue;

            Rect dstChar = Rect(
                (int32_t)(offsetX + x*scale +  (float)xx * scale),
                (int32_t)(offsetY + y*scale + (float)yy * scale),
                (int32_t)ceilf(scale),
                (int32_t)ceilf(scale)
            );

            // Draw the actual pixel at full opacity
            screen.pen = Pen(r,g,b);
            screen.rectangle(dstChar);
        }
    }
}

void md_drawRect(float x, float y, float w, float h, unsigned char r,
                 unsigned char g, unsigned char b) {
    //adjust for different behaviour between sdl and js in case of negative width / height
    if(w < 0.0f) {
        x += w;
        w *= -1.0f;
    }
    if(h < 0.0f) {
        y += h;
        h *= -1.0f;
    }

    Rect rect = Rect(
        (int32_t)(offsetX + x * scale),
        (int32_t)(offsetY + y * scale),
        (int32_t)ceilf(w * scale),
        (int32_t)ceilf(h * scale)
    );

    // Draw the main rectangle
    screen.pen = Pen(r,g,b);
    screen.rectangle(rect);
}

void md_clearView(unsigned char r, unsigned char g, unsigned char b) 
{
    screen.pen = Pen(r,g,b);
    Rect rect = Rect(
        (int32_t)(offsetX),
        (int32_t)(offsetY),
        (int32_t)(viewW),
        (int32_t)(viewH)
    );
    screen.rectangle(rect);
}

void md_clearScreen(unsigned char r, unsigned char g, unsigned char b)
{
    clearColorR = r;
    clearColorG = g;
    clearColorB = b;
    Rect tmpClip = screen.clip;
    screen.clip.x = 0;
    screen.clip.y = 0;
    screen.clip.w = screen.bounds.w;
    screen.clip.h = screen.bounds.h;
    screen.pen = Pen(r,g,b);
    screen.clear();
    screen.clip = tmpClip;
}

void md_initView(int w, int h) 
{   
    WINDOW_WIDTH = screen.bounds.w;
    WINDOW_HEIGHT = screen.bounds.h;
    float wscalex = (float)WINDOW_WIDTH / (float)DEFAULT_WINDOW_WIDTH;
    float wscaley = (float)WINDOW_HEIGHT / (float)DEFAULT_WINDOW_HEIGHT;
    wscale = (wscaley < wscalex) ? wscaley : wscalex;

    origViewW = w;
    origViewH = h;
    float xScale = (float)WINDOW_WIDTH / w;
    float yScale = (float)WINDOW_HEIGHT / h;
    if (yScale < xScale)
        scale = yScale;
    else
        scale = xScale;
    viewW = (int)floorf((float)w * scale);
    viewH = (int)floorf((float)h * scale);
    offsetX = (int)(WINDOW_WIDTH - viewW) >> 1;
    offsetY = (int)(WINDOW_HEIGHT - viewH) >> 1;
    //printf("%d %d %d %d\n", offsetX, offsetY, viewW, viewH);
    screen.clip.x = offsetX;
    screen.clip.y = offsetY;
    screen.clip.w = viewW;
    screen.clip.h = viewH;
    mouseX = viewW >> 1;
    mouseY = viewH >> 1;
}

void md_consoleLog(char* msg) 
{ 
    printf(msg); 
}


void printDebugCpuRamFpsLoad(uint32_t start_frame, uint32_t end_frame)
{
    if(debugMode)
    {
        Rect tmpClip = screen.clip;
        screen.clip.x = 0;
        screen.clip.y = 0;
        screen.clip.w = screen.bounds.w;
        screen.clip.h = screen.bounds.h;        
 #if defined(TARGET_32BLIT_HW) || defined(PICO_BUILD)

        // memory stats
#ifdef TARGET_32BLIT_HW
        extern char _sbss, _end, __ltdc_start;

        auto static_used = &_end - &_sbss;
        auto heap_total = &__ltdc_start - &_end;
#else // pico
        extern char __bss_start__, end, __StackLimit;

        auto static_used = &end - &__bss_start__;
        auto heap_total = &__StackLimit - &end;
#endif

        auto heap_used = mallinfo().uordblks;

        auto total_ram = static_used + heap_total;

        Point pos(0, 0);
        int w = screen.bounds.w;
        int h = 10;

        screen.pen = {128, 128, 128};
        int static_px = static_used * w / total_ram;
        screen.rectangle({pos.x, pos.y, static_px, h});

        screen.pen = {255, 255, 255};
        int heap_px = heap_used * w / total_ram;
        screen.rectangle({pos.x + static_px, pos.y, heap_px, h});

        screen.pen = {64, 64, 64};
        screen.rectangle({pos.x + static_px + heap_px, pos.y, w - (static_px + heap_px), h});

        screen.pen = {0, 0, 0};
        screen.rectangle({pos.x, pos.y + h, w, h});

        screen.pen = {255, 255, 255};
        char buf[100];
        snprintf(buf, sizeof(buf), "Mem: %i + %i / %i", static_used, heap_used, total_ram);
        screen.text(buf, minimal_font, {pos.x, pos.y + h, w, h}, true, TextAlign::center_center);

#endif
        uint32_t us = end_frame - start_frame;
        if (us == 0)
            us = 1;   
        long int fps = 1000000.0 / us;
        char buf2[100];
        snprintf(buf2, sizeof(buf2), "FPS: %ld", fps);
        screen.pen = Pen(255,255,255);
        screen.rectangle(Rect(1, screen.bounds.h - 10,  12 * 6 + 2, 10));
        screen.pen = Pen(0,0,0);
        screen.text(buf2, minimal_font, {1, screen.bounds.h - 9}, true, TextAlign::top_left);
        screen.clip = tmpClip;
    }    
}

void init()
{
    set_screen_mode(hires);
    onSaveData = saveHighScores;
    initGame();
    loadHighScores();
}

void render(uint32_t time) 
{      
    if (now_us() - prevtime < 20000)
        return;

    updateFromSoundTask();

    prevtime = now_us();
    
   
    bool mouseUsed = getGame(currentGameIndex).usesMouse;
    setButtonState(!mouseUsed && (buttons.state & Button::DPAD_LEFT), 
        !mouseUsed && (buttons.state & Button::DPAD_RIGHT), 
        !mouseUsed && (buttons.state & Button::DPAD_UP), 
        !mouseUsed && (buttons.state & Button::DPAD_DOWN), 
        buttons.state & Button::B, buttons.state & Button::A); 
    
    if (mouseUsed)
    {
        if(buttons.state & Button::DPAD_RIGHT)
            mouseX += WINDOW_WIDTH /100.0f;
        
        if(buttons.state & Button::DPAD_LEFT)
            mouseX -= WINDOW_WIDTH /100.0f;
            
        if(buttons.state & Button::DPAD_UP)
            mouseY -= WINDOW_HEIGHT /100.0f;
    
        if(buttons.state & Button::DPAD_DOWN)
            mouseY += WINDOW_HEIGHT /100.0f;

        mouseX = clamp(mouseX, 0, WINDOW_WIDTH - 2*offsetX -1);
        mouseY = clamp(mouseY, 0, WINDOW_HEIGHT - 2*offsetY -1);

        setMousePos(mouseX / scale, mouseY / scale);
    }
    
    updateFrame();

    if(mouseUsed)
    {
        screen.pen = Pen(255, 105, 180);
        Rect dstHorz = Rect((int32_t)offsetX + (mouseX-3*wscale), offsetY + (int32_t)(mouseY-1*wscale), (int32_t)(7.0f*wscale),(int32_t)(3.0f*wscale));
        screen.rectangle(dstHorz);
        Rect dstVert = Rect((int32_t)offsetX + (mouseX-1*wscale), offsetY + (int32_t)(mouseY-3*wscale), (int32_t)(3.0f*wscale),(int32_t)(7.0f*wscale));
        screen.rectangle(dstVert);
    }

    if(buttons.state & Button::Y)
        goToMenu();     
    
    printDebugCpuRamFpsLoad(starttime, now_us());
    starttime = now_us();    
}


void update(uint32_t time) 
{
    
}

