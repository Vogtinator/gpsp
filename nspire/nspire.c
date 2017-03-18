#include "../common.h"

#include <libndls.h>

u16 extra_screen_1[320*240] __attribute__ ((aligned (8)));
u16 extra_screen_2[320*240] __attribute__ ((aligned (8)));

void* nspire_displayed_screen;
void* nspire_screen;
u32 LCDTiming0;
u32 LCDTiming1;
u32 LCDControl;

void nspire_init()
{
	LCDTiming0 = *(volatile u32*)0xC0000000;
	LCDTiming1 = *(volatile u32*)0xC0000004;
	LCDControl = *(volatile u32*)0xC0000018;
	nspire_displayed_screen = extra_screen_1;
	nspire_screen = extra_screen_2;
	*(volatile u32*)0xC0000018 = 0;
	*(volatile u32*)0xC0000000 = 0x251A004C;
	*(volatile u32*)0xC0000004 = 0x110300EF;
	set_display_buffer(nspire_displayed_screen);
	u32 newControl = (LCDControl & ~0x90E) | 0x008;
	*(volatile u32*)0xC0000018 = newControl;
	*(volatile u32*)0xC0000018 = newControl | 0x800;
	lcd_init(SCR_320x240_555);
}

void nspire_restore()
{
	lcd_init(SCR_TYPE_INVALID);
	*(volatile u32*)0xC0000018 = 0;
	*(volatile u32*)0xC0000000 = LCDTiming0;
	*(volatile u32*)0xC0000004 = LCDTiming1;
	*(volatile u32*)0xC0000018 = LCDControl & ~0x800;
	*(volatile u32*)0xC0000018 = LCDControl;
	enableAlignmentExceptions();
	//refresh_osscr();
}

void set_display_buffer(void* buffer)
{
	lcd_blit(buffer, SCR_320x240_555);
}

void update_at_vblank()
{
	while((*(volatile unsigned*)0xC0000020 & 4) == 0) { }
	*(volatile unsigned*)0xC0000028 = 4;
	set_display_buffer(nspire_displayed_screen);
}

//#define VERY_SAFE_CACHE

int warm_cache_op_range(int op, void *addr, unsigned long size)
{
	void *end_addr = addr + size;
	switch (op)
	{
	case WOP_D_CLEAN:
#ifndef VERY_SAFE_CACHE
		addr = (void*)((u32)addr & ~0x1F);
		while(addr < end_addr)
		{
			__asm("MCR p15, 0, %0, c7, c10, 1\n\t" : : "r" (addr) : );
			addr += 32;
		}
		op = 0;
		__asm("MCR p15, 0, %0, c7, c10, 4\n\t" : : "r" (op) : );
#else
		clean_dcache();
#endif
		return 0;
	case WOP_I_INVALIDATE:
#ifndef VERY_SAFE_CACHE
		addr = (void*)((u32)addr & ~0x1F);
		while(addr < end_addr)
		{
			__asm("MCR p15, 0, %0, c7, c5, 1\n\t" : : "r" (addr) : );
			addr += 32;
		}
#else
		op = 0;
		__asm("MCR p15, 0, %0, c7, c5, 0\n\t" : : "r" (op) : );
#endif
		return 0;
	}
	return -1;
}

unsigned months[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

struct tm* localtime(const time_t* input_time)
{
	unsigned int time = *input_time;
	
	static struct tm tm_result;
	tm_result.tm_isdst = 0;
	
	tm_result.tm_sec = time % 60;
	time /= 60;
	
	tm_result.tm_min = time % 60;
	time /= 60;
	
	tm_result.tm_hour = time % 24;
	time /= 24;
	
	tm_result.tm_wday = (time + 4) % 7;
	
	unsigned leap = 0;
	tm_result.tm_year = 70 + (time / (365*4 + 1))*4;
	time %= (365*4 + 1);
	if (time >= (365*3 + 1))
	{
		time -= (365*3 + 1);
		tm_result.tm_year += 3;
	}
	else if (time >= 365*2)
	{
		time -= 365*2;
		tm_result.tm_year += 2;
		leap = 1;
	}
	else if (time >= 365)
	{
		time -= 365;
		tm_result.tm_year++;
	}
	tm_result.tm_yday = time;
	if (leap && time >= 31 + 29 - 1)
	{
		if (time == 31 + 29 - 1)
		{
			tm_result.tm_mon = 1;
			tm_result.tm_mday = 29;
			return &tm_result;
		}
		time--;
	}
	
	tm_result.tm_mon = 0;
	while(time >= months[tm_result.tm_mon])
		time -= months[tm_result.tm_mon++];
	tm_result.tm_mday = time + 1;
	return &tm_result;
}
