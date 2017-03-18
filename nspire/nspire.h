#define getcwd(path, size) \
	NU_Current_Dir("A:", (char*)(path))
	
#define GetRTC() (*(volatile unsigned*)0x90090000)

#define WOP_D_CLEAN 0
#define WOP_I_INVALIDATE 1

extern void* nspire_screen;
extern void* nspire_screen_2;
extern void* nspire_screen_3;
extern void* nspire_displayed_screen;
extern u16* os_screen;
void nspire_init();
void nspire_restore();
void set_display_buffer(void* buffer);
void update_at_vblank();

int warm_cache_op_range(int op, void *addr, unsigned long size);
void clean_dcache();

struct tm
{
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

void time(time_t*);

struct tm* localtime(const time_t*);

void upscale_aspect(u16 *dst, u16 *src);
void upscale_aspect_fast(u16 *dst, u16 *src);
void upscale_aspect_raw(u16 *dst, u16 *src);

int strcasecmp(const char* s1, const char* s2);

void enableAlignmentExceptions();
u32 menu_wrapper(u16*);
u32 save_backup_wrapper(char*);
void save_config_file_wrapper();
void load_state_wrapper(u8*);
void save_state_wrapper(u8*, u16*);
void quit_wrapper();

u32 key_scan();