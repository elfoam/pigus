#pragma once

#include <cstdint>

#include <circle/actled.h>
#include <circle/multicore.h>
#include <circle/memory.h>
#include <circle/types.h>
#include <circle/pwmsoundbasedevice.h>
#include <circle/logger.h>
#include <circle/spinlock.h>
#include <circle/timer.h>
#include <circle/gpiopin.h>

#define IPI_IOW 10
#define IPI_IOR 11

#define SNDBUFSIZ 256

class SoundcardEmu : public CMultiCoreSupport
{
public:
	SoundcardEmu(CMemorySystem* pMemorySystem, CInterruptSystem* pInterrupt, CSpinLock &spinlock);
	~SoundcardEmu(void);

	virtual boolean Initialize(void);
	void Run(unsigned nCore);
	virtual TGPIOInterruptHandler* getIOWInterruptHandler() = 0;
	virtual TGPIOInterruptHandler* getIORInterruptHandler() = 0;

protected:
	CLogger	&m_Logger;
	CInterruptSystem* m_pInterrupt;
	CSpinLock &m_SpinLock;
#ifndef USE_INTERRUPTS
	u32 gpios;
#endif

	void IPIHandler(unsigned nCore, unsigned nIPI) override;
	virtual void RenderSound(s16* buffer, size_t nFrames) = 0;

	static void FastGPIOWriteData(u8 value, boolean setOutput = FALSE);
	static void FastGPIOClear(void);
	static u32 FastGPIORead(void);

private:
	CPWMSoundBaseDevice *m_sndDevice;

	void MainTask(void);
	void SoundTask(void); 
	void IOTask(void); 
};
