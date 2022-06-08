#include <circle/sysconfig.h>
#include <circle/memorymap.h>
#include <circle/gpiopin.h>

#include "gusemu.h"

GusEmu::GusEmu(CMemorySystem* pMemorySystem, CInterruptSystem* pInterrupt, CSpinLock &spinlock)
:
    SoundcardEmu(pMemorySystem, pInterrupt, spinlock)
{
    gus = std::make_unique<Gus>(GUS_PORT, 1, 5, "", m_Logger);
    for (size_t i = 0; i < m_DataPins.size(); i++) {
        m_DataPins[i] = new CGPIOPin(i, GPIOModeInput);
    }
}

GusEmu::~GusEmu(void) {}


boolean GusEmu::Initialize(void) 
{
    m_Logger.Write("GusEmu", LogNotice, "init GUS");

    if (!SoundcardEmu::Initialize()) {
        return FALSE;
    }

    return TRUE;
}


void GusEmu::RenderSound(s16* buffer, size_t nFrames)
{
    // enjoy the silence
    memset(buffer, 0, nFrames * 2 * sizeof(s16));
//    adlib_getsample(buffer, nFrames);
}


TGPIOInterruptHandler* GusEmu::getIOWInterruptHandler()
{
    return GusEmu::HandleIOWInterrupt;
}


TGPIOInterruptHandler* GusEmu::getIORInterruptHandler()
{
    return GusEmu::HandleIORInterrupt;
}


void GusEmu::HandleIOWInterrupt(void *pParam)
{
    GusEmu* pThis = static_cast<GusEmu*>(pParam);
    /* pThis->m_Logger.Write("GusEmu", LogNotice, "IOW"); */
#ifdef USE_INTERRUPTS
    u32 gpios = CGPIOPin::ReadAll();
    /* u32 gpios = SoundcardEmu::FastGPIORead(); */
#else
    u32 gpios = pThis->gpios;
#endif
    io_port_t port = ((gpios >> 12) & 0x3FF) - GUS_PORT_BASE;
    io_val_t value;
    switch (port) {
        case 0x302:
        case 0x303:
        case 0x304: // 3x4 supports 16-bit transfers but PiGUS doesn't! force byte
        case 0x305:
        case 0x208:
        case 0x209:
        case 0x307:
        // Board Only
        case 0x200:
        case 0x20b:
            value = (gpios >> 4) & 0xFF;
            // let's a go
            pThis->gus->WriteToPort(port, value, io_width_t::byte);
            CActLED::Get()->Blink(1);
            break;
    }
}


void GusEmu::HandleIORInterrupt(void *pParam)
{
    GusEmu* pThis = static_cast<GusEmu*>(pParam);
#ifdef USE_INTERRUPTS
    /* u32 gpios = CGPIOPin::ReadAll(); */
    u32 gpios = SoundcardEmu::FastGPIORead();
#else
    u32 gpios = pThis->gpios;
#endif

    io_port_t port = ((gpios >> 12) & 0x3FF) - GUS_PORT_BASE;
    /* pThis->m_Logger.Write("GusEmu", LogNotice, "IOR port %d", port); */
    uint8_t value;
    switch (port) {
        case 0x302:
        case 0x303:
        case 0x304:
        case 0x305:
        case 0x206:
        case 0x208:
        // Board Only
        case 0x20a:
        case 0x307:
            if (!(gpios & 0x2)) {
                // falling edge - read data
                /*
                for (size_t i = 0; i < 8; ++i) {
                    pThis->m_DataPins[i]->SetMode(GPIOModeOutput);
                }
                */
                //value = pThis->gus->ReadFromPort(port, io_width_t::byte);
                value = 0x55;
                /* CGPIOPin::WriteAll(value << 4, 0xFF0); */
                SoundcardEmu::FastGPIOWriteData(value, TRUE);
                /* value = 0xAA; */
                /* CGPIOPin::WriteAll(value << 4, 0xFF0); */
                /* SoundcardEmu::FastGPIOWriteData(value); */
                /* CActLED::Get()->On(); */
                /*
                CTimer::SimpleusDelay(1);
                SoundcardEmu::FastGPIOClear();
                */
            } else {
                // rising edge - Set data pins back to inputs
                /*
                for (unsigned i=4; i < 12; ++i) {
                    CGPIOPin pin(i, GPIOModeInput);
                }
                for (size_t i = 0; i < 8; ++i) {
                    pThis->m_DataPins[i]->SetMode(GPIOModeInput);
                }
                */
                /* SoundcardEmu::FastGPIOClear(); */
                /* CActLED::Get()->Off(); */
            }
            break;
    }
}


