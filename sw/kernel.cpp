#include "kernel.h"
//#include <circle/timer.h>
#include <cstddef>

#include <circle/gpiopin.h>
#include <circle/gpiopinfiq.h>
#include <circle/pwmsoundbasedevice.h>
#include <circle/synchronize.h>
#include <circle/multicore.h>

//#define EMULATE_ADLIB

#ifdef EMULATE_ADLIB
#include "adlibemu.h"
#else
#include "gusemu.h"
#endif

CKernel::CKernel(void)
:
    /* m_CPUThrottle(CPUSpeedLow), */
    m_Timer(&m_Interrupt),
    m_Logger(4/*, &m_Timer*/),
    m_Screen(0, 0),
    m_Manager(&m_Interrupt)
{
#ifdef EMULATE_ADLIB
    m_pSoundcardEmu = new AdlibEmu(CMemorySystem::Get(), &m_Interrupt, *m_SpinLock);
#else
    m_pSoundcardEmu = new GusEmu(CMemorySystem::Get(), &m_Interrupt, *m_SpinLock);
#endif
}

CKernel::~CKernel(void)
{
}

boolean CKernel::Initialize(void)
{
    m_Screen.Initialize();
    CDevice *pTarget = m_DeviceNameService.GetDevice (
            m_Options.GetLogDevice (), FALSE);
    if (pTarget == 0) {
        pTarget = &m_Screen;
    }
    m_Logger.Initialize(pTarget);
    m_Interrupt.Initialize();
    m_Manager.Initialize();
    m_pSoundcardEmu->Initialize();
    m_CPUThrottle.SetSpeed(CPUSpeedMaximum);
    return TRUE;
}

TShutdownMode CKernel::Run(void)
{
    m_Logger.Write("kernel", LogNotice, "init GPIO");
#ifdef USE_INTERRUPTS
    CGPIOPin iow_pin(0, GPIOModeInput, &m_Manager);
    CGPIOPinFIQ ior_pin(1, GPIOModeInput, &m_Interrupt);
    /*
    CGPIOPinFIQ iow_pin(0, GPIOModeInput, &m_Interrupt);
    CGPIOPin ior_pin(1, GPIOModeInput, &m_Manager);
    */
#else
    CGPIOPin iow_pin(0, GPIOModeInput);
    CGPIOPin ior_pin(1, GPIOModeInput);
#endif
    for (unsigned i=2; i < 32; ++i) {
        CGPIOPin pin(i, i == 27 ? GPIOModeOutput : GPIOModeInput);
        if (i == 27) {
            // Enable level shifters
            pin.Write(1);
        }
    }


#ifdef USE_INTERRUPTS
    m_Logger.Write("kernel", LogNotice, "set up interrupts");
    TGPIOInterruptHandler* iowHandler = m_pSoundcardEmu->getIOWInterruptHandler();
    if (iowHandler) {
	iow_pin.ConnectInterrupt(iowHandler, m_pSoundcardEmu);
	iow_pin.EnableInterrupt(GPIOInterruptOnFallingEdge);
    }
    TGPIOInterruptHandler* iorHandler = m_pSoundcardEmu->getIORInterruptHandler();
    if (iorHandler) {
	ior_pin.ConnectInterrupt(iorHandler, m_pSoundcardEmu);
	ior_pin.EnableInterrupt(GPIOInterruptOnFallingEdge);
	ior_pin.EnableInterrupt2(GPIOInterruptOnRisingEdge);
    }
#endif

    m_Logger.Write("kernel", LogNotice, "Running SoundcardEmu");
    m_pSoundcardEmu->Run(0);

    return ShutdownReboot;
}


