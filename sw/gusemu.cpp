#include <circle/sysconfig.h>
#include <circle/memorymap.h>
#include <circle/gpiopin.h>

#include "gusemu.h"
#include "gpio.h"

GusEmu::GusEmu(CMemorySystem* pMemorySystem, CInterruptSystem* pInterrupt, CTimer &timer)
:
    SoundcardEmu(pMemorySystem, pInterrupt, timer)
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
    /* memset(buffer, 0, nFrames * 2 * sizeof(s16)); */
//    adlib_getsample(buffer, nFrames);
    std::vector<int16_t> v_buffer(nFrames * 2);
    gus->AudioCallback(nFrames, v_buffer);
    // uuugggh copying
    memcpy(buffer, v_buffer.data(), nFrames * 2 * sizeof(s16));
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
    /* u32 gpios = CGPIOPin::ReadAll(); */
    u32 gpios = FastGPIO::FastGPIORead();
#else
    u32 gpios = pThis->gpios;
#endif
    io_port_t port = ((gpios >> 12) & 0x3FF) - GUS_PORT_BASE;
    io_val_t value;
    switch (port) {
        case 0x200:
        case 0x208:
        case 0x209:
        case 0x20b:
        case 0x302:
        case 0x303:
        case 0x304:
        case 0x305:
        case 0x307:
            value = (gpios >> 4) & 0xFF;
            // let's a go
            pThis->gus->WriteToPort(port + GUS_PORT_BASE, value, io_width_t::byte); // 3x4 supports 16-bit transfers but PiGUS doesn't! force byte
            break;
    }
}


void GusEmu::HandleIORInterrupt(void *pParam)
{
    GusEmu* pThis = static_cast<GusEmu*>(pParam);
#if defined USE_INTERRUPTS || defined USE_HYBRID_POLLING
    /* u32 gpios = CGPIOPin::ReadAll(); */
    u32 gpios = FastGPIO::FastGPIORead();
#else
    u32 gpios = pThis->gpios;
#endif

    io_port_t port = ((gpios >> 12) & 0x3FF) - GUS_PORT_BASE;
    /* pThis->m_Logger.Write("GusEmu", LogNotice, "IOR port %d", port); */
    uint8_t value;
    switch (port) {
        case 0x206:
        case 0x208:
        case 0x20a:
        case 0x302:
        case 0x303:
        case 0x304:
        case 0x305:
        case 0x307:
            value = pThis->gus->ReadFromPort(port + GUS_PORT_BASE, io_width_t::byte);
            FastGPIO::FastGPIOWriteData(value, TRUE);
            pThis->m_Timer.nsDelay(1500);
            FastGPIO::FastGPIOClear();
            break;
    }
}


#ifndef USE_INTERRUPTS
void GusEmu::IOTask(void) {
    m_Logger.Write("GusEmu", LogNotice, "IOTask starting up (overridden)");

    u8 curr_iow, last_iow = 0x1, curr_ior, last_ior = 0x2;

    for (;;) {
	/* gpios = CGPIOPin::ReadAll(); */
        gpios = FastGPIO::FastGPIORead();
	curr_iow = gpios & 0x1;
#ifndef USE_HYBRID_POLLING
	curr_ior = gpios & 0x2;
#endif

        u16 port = ((gpios >> 12) & 0x3FF) - GUS_PORT_BASE;
        if (!(port & 0x200)) {
            continue;
        }

	if (curr_iow < last_iow) {  // falling edge of ~IOW
            HandleIOWInterrupt(this);
	    /* CMultiCoreSupport::SendIPI(3, IPI_IOW); */
	}
#ifndef USE_HYBRID_POLLING
	// Be careful of race conditions here -- should this be an else if?
	if (curr_ior < last_ior) {  // falling edge of ~IOR
            HandleIORInterrupt(this);
	    /* CMultiCoreSupport::SendIPI(3, IPI_IOR); */
	}
#endif

	last_iow = curr_iow;
#ifndef USE_HYBRID_POLLING
	last_ior = curr_ior;
#endif
    }
}
#endif
