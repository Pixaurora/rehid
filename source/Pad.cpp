#include "Pad.hpp"
#include "printf.h"
#include "irrst.hpp"
#include "3ds/os.h"
extern "C"{
#include "gpio.h"
}
extern void _putchar(char character);
/*
{
    svcOutputDebugString(&character, 1);
}
*/
int time_to_end_macro = 0;

void Pad::Initialize()
{
    if(!m_isinitialized)
    {
        m_isinitialized = true;
        m_latestkeys = (vu32)(IOHIDPAD) ^ 0xFFF;
        m_dummy = 0x4001;
        m_slider.GetConfigSettings();
        m_circlepad.GetConfigSettings();
        svcCreateTimer(&m_timer, RESET_ONESHOT);
        svcCreateEvent(&m_event, RESET_ONESHOT);
    }
}

void Pad::SetTimer()
{
    if(R_FAILED(svcSetTimer(m_timer, 4000000LL, 0LL)))
        svcBreak(USERBREAK_ASSERT);
}

extern u8 irneeded;
extern u32 debugpadkeys;
extern CirclePadEntry debugpadstick;
void Pad::ReadFromIO(PadEntry *entry, uint32_t *raw, CirclePadEntry *circlepad, Remapper *remapper)
{
    volatile uint32_t latest = (vu32)(IOHIDPAD) ^ 0xFFF;
    
    uint32_t val = 0;
    GPIOHID_GetData(0x4001, &val);
    *raw = latest;
    latest = latest & ~(2 * (latest & 0x40) | ((latest & 0x20u) >> 1));
    // if(val & 0x1) {
    //     latest |= debugpadkeys;
    //     circlepad->x = debugpadstick.x;
    //     circlepad->y = debugpadstick.y;
    // }
    latest = m_circlepad.ConvertToHidButtons(circlepad, latest, remapper); // if need be this also sets the circlepad entry to 0
    if(irneeded == 1){
        iruScanInput_();
        m_rawkeys = iruKeysHeld_();
    }
    latest = latest | m_rawkeys | remapper->m_remaptouchkeys;
    latest = remapper->Remap(latest);
    entry->pressedpadstate = (latest ^ m_latestkeys) & ~m_latestkeys;
    entry->releasedpadstate = (latest ^ m_latestkeys) & m_latestkeys;
    entry->currpadstate = latest;
    if(latest & KEY_ZL) {
        time_to_end_macro = osGetTime() + 120000;
    }

    if(latest & KEY_ZR) {
        time_to_end_macro = osGetTime();
    }

    if (osGetTime() < time_to_end_macro) {
        latest |= KEY_CPAD_LEFT;
        latest |= KEY_A;
    }
    m_latestkeys = latest;
}

void Pad::Sampling(u32 rcpr, Remapper *remapper)
{
    PadEntry finalentry; uint32_t latest;
    static float sliderval = 0.0f;
    CirclePadEntry rawcirclepad;
    rawcirclepad.x = rcpr & 0xFFF;
    rawcirclepad.y = (rcpr & 0xFFF000) >> 12;
    CirclePadEntry finalcirclepad;
    svcSetTimer(m_timer, 4000000LL, 0LL);
    m_circlepad.RawToCirclePadCoords(&finalcirclepad, rawcirclepad);
    if(m_counter % 3 == 0)
    {
        m_slider.ReadValuesFromMCU();
        sliderval = m_slider.Normalize();
        *(float*)0x1FF81080 = sliderval;
    }
    ++m_counter;
    ReadFromIO(&finalentry, &latest, &finalcirclepad, remapper);
    m_ring->SetCurrPadState(latest, rawcirclepad);
    m_ring->Set3dSliderVal(sliderval);
    m_ring->WriteToRing(&finalentry, &finalcirclepad);
    svcSignalEvent(m_event);
}