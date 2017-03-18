/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "common.h"

#ifdef PSP_BUILD

//PSP_MODULE_INFO("gpSP", 0x1000, 0, 6);
//PSP_MAIN_THREAD_ATTR(THREAD_ATTR_USER);

void vblank_interrupt_handler(u32 sub, u32 *parg);

#endif

timer_type timer[4];

//debug_state current_debug_state = COUNTDOWN_BREAKPOINT;
//debug_state current_debug_state = PC_BREAKPOINT;
u32 breakpoint_value = 0x7c5000;
debug_state current_debug_state = RUN;
//debug_state current_debug_state = STEP_RUN;

//u32 breakpoint_value = 0;

frameskip_type current_frameskip_type = auto_frameskip;
u32 global_cycles_per_instruction = 1;
u32 random_skip = 0;

#ifdef GP2X_BUILD
u32 frameskip_value = 2;

u64 frame_count_initial_timestamp = 0;
u64 last_frame_interval_timestamp;
u32 gp2x_fps_debug = 0;

void gp2x_init(void);
void gp2x_quit(void);
#else

u32 gp2x_fps_debug = 0;
u32 frameskip_value = 4;
#endif
u32 skip_next_frame = 0;

u32 frameskip_counter = 0;

u32 cpu_ticks = 0;
u32 frame_ticks = 0;

u32 execute_cycles = 960;
s32 video_count = 960;
u32 ticks;

u32 arm_frame = 0;
u32 thumb_frame = 0;
u32 last_frame = 0;

u32 cycle_memory_access = 0;
u32 cycle_pc_relative_access = 0;
u32 cycle_sp_relative_access = 0;
u32 cycle_block_memory_access = 0;
u32 cycle_block_memory_sp_access = 0;
u32 cycle_block_memory_words = 0;
u32 cycle_dma16_words = 0;
u32 cycle_dma32_words = 0;
u32 flush_ram_count = 0;
u32 gbc_update_count = 0;
u32 oam_update_count = 0;

u32 synchronize_flag = 1;

u32 update_backup_flag = 1;
#ifdef GP2X_BUILD
u32 clock_speed = 200;
#else
u32 clock_speed = 333;
#endif
u32 quick_save_slot = 0;
u8 main_path[512];

void trigger_ext_event();

#define check_count(count_var)                                                \
  if(count_var < (s32)execute_cycles)                                              \
    execute_cycles = count_var;                                               \

#define check_timer(timer_number)                                             \
  if(timer[timer_number].status == TIMER_PRESCALE)                            \
    check_count(timer[timer_number].count);                                   \

#define update_timer(timer_number)                                            \
  if(timer[timer_number].status != TIMER_INACTIVE)                            \
  {                                                                           \
    if(timer[timer_number].status != TIMER_CASCADE)                           \
    {                                                                         \
      timer[timer_number].count -= execute_cycles;                            \
      io_registers[REG_TM##timer_number##D] =                                 \
       -(timer[timer_number].count >> timer[timer_number].prescale);          \
    }                                                                         \
                                                                              \
    if(timer[timer_number].count <= 0)                                        \
    {                                                                         \
      if(timer[timer_number].irq == TIMER_TRIGGER_IRQ)                        \
        irq_raised |= IRQ_TIMER##timer_number;                                \
                                                                              \
      if((timer_number != 3) &&                                               \
       (timer[timer_number + 1].status == TIMER_CASCADE))                     \
      {                                                                       \
        timer[timer_number + 1].count--;                                      \
        io_registers[REG_TM0D + (timer_number + 1) * 2] =                     \
         -(timer[timer_number + 1].count);                                    \
      }                                                                       \
                                                                              \
      if(timer_number < 2)                                                    \
      {                                                                       \
        if(timer[timer_number].direct_sound_channels & 0x01)                  \
          sound_timer(timer[timer_number].frequency_step, 0);                 \
                                                                              \
        if(timer[timer_number].direct_sound_channels & 0x02)                  \
          sound_timer(timer[timer_number].frequency_step, 1);                 \
      }                                                                       \
                                                                              \
      timer[timer_number].count +=                                            \
       (timer[timer_number].reload << timer[timer_number].prescale);          \
    }                                                                         \
  }                                                                           \

#ifdef NSPIRE_BUILD
u8 *file_ext[] = { (u8*)".gba.tns", (u8*)".zip.tns", NULL };
#else
u8 *file_ext[] = { ".gba", ".bin", ".zip", NULL };
#endif

#ifdef ARM_ARCH
void ChangeWorkingDirectory(char *exe)
{
#ifndef _WIN32_WCE
  char *s = strrchr(exe, '/');
  if (s != NULL) {
    *s = '\0';
    chdir(exe);
    *s = '/';
  }
#endif
}

static void switch_to_romdir(void)
{
  char buff[256];
  int r;
  
  file_open(romdir_file, "gpsp_romdir.tns", read);

  if(file_check_valid(romdir_file))
  {
    r = file_read(romdir_file, buff, sizeof(buff) - 1);
    if (r > 0)
    {
      buff[r] = 0;
      while (r > 0 && isspace(buff[r-1]))
        buff[--r] = 0;
      chdir(buff);
    }
    file_close(romdir_file);
  }
}

static void save_romdir(void)
{
  char buff[512];

  sprintf(buff, "%s/gpsp_romdir.tns", main_path);
  file_open(romdir_file, buff, write);

  if(file_check_valid(romdir_file))
  {
    if (!getcwd(buff, sizeof(buff)))
    {
      file_write(romdir_file, buff, strlen(buff));
    }
    file_close(romdir_file);
  }
}
#endif

void init_main()
{
  u32 i;

  skip_next_frame = 0;

  for(i = 0; i < 4; i++)
  {
    dma[i].start_type = DMA_INACTIVE;
    dma[i].direct_sound_channel = DMA_NO_DIRECT_SOUND;
    timer[i].status = TIMER_INACTIVE;
    timer[i].reload = 0x10000;
    timer[i].stop_cpu_ticks = 0;
  }

  timer[0].direct_sound_channels = TIMER_DS_CHANNEL_BOTH;
  timer[1].direct_sound_channels = TIMER_DS_CHANNEL_NONE;

  cpu_ticks = 0;
  frame_ticks = 0;

  execute_cycles = 960;
  video_count = 960;

  flush_translation_cache_rom();
  flush_translation_cache_ram();
  flush_translation_cache_bios();
}

#ifdef NSPIRE_BUILD
s32 relative_frame_count;

int real_main(int argc, char *argv[])
{
  if (is_classic)
	return 0;
#else
int main(int argc, char *argv[])
{
#endif
  
  u32 i;
  u32 vcount = 0;
  u32 ticks;
  u32 dispstat;
  u8 load_filename[512];
  u8 bios_filename[512];

#ifdef PSP_BUILD
  sceKernelRegisterSubIntrHandler(PSP_VBLANK_INT, 0,
   vblank_interrupt_handler, NULL);
  sceKernelEnableSubIntr(PSP_VBLANK_INT, 0);
#elif !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
  freopen("CON", "wb", stdout);
#endif

  extern char *cpu_mode_names[];

  init_gamepak_buffer();

  // Copy the directory path of the executable into main_path

#ifdef ARM_ARCH
  // ChangeWorkingDirectory will null out the filename out of the path
  ChangeWorkingDirectory(argv[0]);
#endif

  getcwd(main_path, 512);
  load_config_file();

  gamepak_filename[0] = 0;

#ifdef PSP_BUILD
  delay_us(2500000);
#endif

#ifdef GP2X_BUILD
  // Overclocking GP2X and MMU patch goes here
  gp2x_init();
#endif

#ifdef NSPIRE_BUILD
  nspire_init();
#endif

  init_video();

#ifdef NSPIRE_BUILD
  if(load_bios("gba_bios.bin.tns") == -1)
#else
#ifdef GP2X_BUILD
  sprintf(bios_filename, "%s/%s", main_path, "gba_bios.bin");
  if(load_bios(bios_filename) == -1)
#else
  if(load_bios("gba_bios.bin") == -1)
#endif
#endif
  {
    gui_action_type gui_action = CURSOR_NONE;
    debug_screen_start();
    debug_screen_printl("Sorry, but gpSP requires a Gameboy Advance BIOS   ");
    debug_screen_printl("image to run correctly. Make sure to get an       ");
    debug_screen_printl("authentic one, it'll be exactly 16384 bytes large ");
    debug_screen_printl("and should have the following md5sum value:       ");
    debug_screen_printl("                                                  ");
    debug_screen_printl("a860e8c0b6d573d191e4ec7db1b1e4f6                  ");
    debug_screen_printl("                                                  ");
    debug_screen_printl("When you do get it name it gba_bios.bin.tns and   ");
    debug_screen_printl("put it in the same directory as gpSP.             ");
    debug_screen_printl("                                                  ");
    debug_screen_printl("Press any button to exit.                         ");

    debug_screen_update();

    while(gui_action == CURSOR_NONE)
    {
      gui_action = get_gui_input();
      delay_us(15000);
    }

    debug_screen_end();

    quit();
  }

  if(bios_rom[0] != 0x18)
  {
    gui_action_type gui_action = CURSOR_NONE;

    debug_screen_start();
    debug_screen_printl("You have an incorrect BIOS image.                 ");
    debug_screen_printl("While many games will work fine, some will not. It");
    debug_screen_printl("is strongly recommended that you obtain the       ");
    debug_screen_printl("correct BIOS file. Do NOT report any bugs if you  ");
    debug_screen_printl("are seeing this message.                          ");
    debug_screen_printl("                                                  ");
    debug_screen_printl("Press any button to resume, at your own risk.     ");

    debug_screen_update();

    while(gui_action == CURSOR_NONE)
    {
      gui_action = get_gui_input();
      delay_us(15000);
    }

    debug_screen_end();
  }

  init_main();
  init_sound();

  init_input();

  video_resolution_large();

  if(argc > 1)
  {
    if(load_gamepak(argv[1]) == -1U)
    {
#ifdef PC_BUILD
      printf("Failed to load gamepak %s, exiting.\n", load_filename);
#endif
      quit();
    }

    set_gba_resolution(screen_scale);
    video_resolution_small();

    init_cpu();
    init_memory();
  }
  else
  {
    switch_to_romdir();
	if(load_file(file_ext, load_filename) == -1)
    {
      menu(copy_screen());
    }
    else
    {
	  if(load_gamepak((char*)load_filename) == -1U)
      {
#ifdef PC_BUILD
        printf("Failed to load gamepak %s, exiting.\n", load_filename);
#endif
        quit();
      }
      set_clock_speed();
      set_gba_resolution(screen_scale);
      video_resolution_small();

      init_cpu();
      init_memory();
    }
  }

#ifdef NSPIRE_BUILD
    //change_ext(gamepak_filename, load_filename, ".qsv.tns");
    //load_state(load_filename);
    //remove(load_filename);
	if (quick_save_slot)
	{
		u8 current_savestate_filename[512];
		get_savestate_filename_noshot(quick_save_slot - 1, current_savestate_filename);
		load_state(current_savestate_filename);
		quick_save_slot = 0;
	}
#endif
  
  last_frame = 0;

  // We'll never actually return from here.

#ifdef PSP_BUILD
  execute_arm_translate(execute_cycles);
#else

#ifdef GP2X_BUILD
  get_ticks_us(&frame_count_initial_timestamp);
#endif

/*  u8 current_savestate_filename[512];
  get_savestate_filename_noshot(savestate_slot,
   current_savestate_filename);
  load_state(current_savestate_filename); */

//  debug_on();

  if(argc > 2)
  {
    current_debug_state = COUNTDOWN_BREAKPOINT;
    breakpoint_value = strtol(argv[2], NULL, 16);
  }

  trigger_ext_event();
  
  execute_arm_translate(execute_cycles);
  execute_arm(execute_cycles);
#endif
  return 0;
}

void print_memory_stats(u32 *counter, u32 *region_stats, char *stats_str)
{
  u32 other_region_counter = region_stats[0x1] + region_stats[0xE] +
   region_stats[0xF];
  u32 rom_region_counter = region_stats[0x8] + region_stats[0x9] +
   region_stats[0xA] + region_stats[0xB] + region_stats[0xC] +
   region_stats[0xD];
  u32 _counter = *counter;

  printf("memory access stats: %s (out of %d)\n", stats_str, (int)_counter);
  printf("bios: %f%%\tiwram: %f%%\tewram: %f%%\tvram: %f\n",
   region_stats[0x0] * 100.0 / _counter, region_stats[0x3] * 100.0 /
   _counter,
   region_stats[0x2] * 100.0 / _counter, region_stats[0x6] * 100.0 /
   _counter);

  printf("oam: %f%%\tpalette: %f%%\trom: %f%%\tother: %f%%\n",
   region_stats[0x7] * 100.0 / _counter, region_stats[0x5] * 100.0 /
   _counter,
   rom_region_counter * 100.0 / _counter, other_region_counter * 100.0 /
   _counter);

  *counter = 0;
  memset(region_stats, 0, sizeof(u32) * 16);
}

u32 event_cycles = 0;
const u32 event_cycles_trigger = 60 * 5;
u32 no_alpha = 0;

void trigger_ext_event()
{
  static u32 event_number = 0;
  static u64 benchmark_ticks[16];
  u64 new_ticks;
  u8 current_savestate_filename[512];

  return;

  if(event_number)
  {
    get_ticks_us(&new_ticks);
    benchmark_ticks[event_number - 1] =
     new_ticks - benchmark_ticks[event_number - 1];
  }

  current_frameskip_type = no_frameskip;
  no_alpha = 0;
  synchronize_flag = 0;

  get_savestate_filename_noshot(savestate_slot,
   current_savestate_filename);
  load_state((char*)current_savestate_filename);

  switch(event_number)
  {
    case 0:
      // Full benchmark, run normally
      break;

    case 1:
      // No alpha blending
      no_alpha = 1;
      break;

    case 2:
      // No video benchmark
      // Set frameskip really high + manual
      current_frameskip_type = manual_frameskip;
      frameskip_value = 1000000;
      break;

    case 3:
      // No CPU benchmark
      // Put CPU in halt mode, put it in IRQ mode with interrupts off
      reg[CPU_HALT_STATE] = CPU_HALT;
      reg[REG_CPSR] = 0xD2;
      break;

    case 4:
      // No CPU or video benchmark
      reg[CPU_HALT_STATE] = CPU_HALT;
      reg[REG_CPSR] = 0xD2;
      current_frameskip_type = manual_frameskip;
      frameskip_value = 1000000;
      break;

    case 5:
    {
      // Done
      char *print_strings[] =
      {
        "Full test   ",
        "No blending ",
        "No video    ",
        "No CPU      ",
        "No CPU/video",
        "CPU speed   ",
        "Video speed ",
        "Alpha cost  "
      };
      u32 i;

      benchmark_ticks[6] = benchmark_ticks[0] - benchmark_ticks[2];
      benchmark_ticks[5] = benchmark_ticks[0] - benchmark_ticks[4] -
       benchmark_ticks[6];
      benchmark_ticks[7] = benchmark_ticks[0] - benchmark_ticks[1];

      printf("Benchmark results (%d frames): \n", (int)event_cycles_trigger);
      for(i = 0; i < 8; i++)
      {
        printf("   %s: %d ms (%f ms per frame)\n",
         print_strings[i], (int)benchmark_ticks[i] / 1000,
         (float)(benchmark_ticks[i] / (1000.0 * event_cycles_trigger)));
        if(i == 4)
          printf("\n");
      }
      quit();
    }
  }

  event_cycles = 0;

  get_ticks_us(benchmark_ticks + event_number);
  event_number++;
}

static u32 fps = 60;
static u32 frames_drawn = 60;

u32 update_gba()
{
  irq_type irq_raised = IRQ_NONE;

  do
  {
    cpu_ticks += execute_cycles;

    reg[CHANGED_PC_STATUS] = 0;

    if(gbc_sound_update)
    {
      gbc_update_count++;
      update_gbc_sound(cpu_ticks);
      gbc_sound_update = 0;
    }

    update_timer(0);
    update_timer(1);
    update_timer(2);
    update_timer(3);

    video_count -= execute_cycles;

    if(video_count <= 0)
    {
      u32 vcount = io_registers[REG_VCOUNT];
      u32 dispstat = io_registers[REG_DISPSTAT];

      if((dispstat & 0x02) == 0)
      {
        // Transition from hrefresh to hblank
        video_count += (272);
        dispstat |= 0x02;

        if((dispstat & 0x01) == 0)
        {
          u32 i;
          if(oam_update)
            oam_update_count++;

          if(no_alpha)
            io_registers[REG_BLDCNT] = 0;
          update_scanline();

          // If in visible area also fire HDMA
          for(i = 0; i < 4; i++)
          {
            if(dma[i].start_type == DMA_START_HBLANK)
              dma_transfer(dma + i);
          }
        }

        if(dispstat & 0x10)
          irq_raised |= IRQ_HBLANK;
      }
      else
      {
        // Transition from hblank to next line
        video_count += 960;
        dispstat &= ~0x02;

        vcount++;

        if(vcount == 160)
        {
          // Transition from vrefresh to vblank
          u32 i;

          dispstat |= 0x01;
          if(dispstat & 0x8)
          {
            irq_raised |= IRQ_VBLANK;
          }

          affine_reference_x[0] =
           (s32)(address32(io_registers, 0x28) << 4) >> 4;
          affine_reference_y[0] =
           (s32)(address32(io_registers, 0x2C) << 4) >> 4;
          affine_reference_x[1] =
           (s32)(address32(io_registers, 0x38) << 4) >> 4;
          affine_reference_y[1] =
           (s32)(address32(io_registers, 0x3C) << 4) >> 4;

          for(i = 0; i < 4; i++)
          {
            if(dma[i].start_type == DMA_START_VBLANK)
              dma_transfer(dma + i);
          }
        }
        else

        if(vcount == 228)
        {
          // Transition from vblank to next screen
          dispstat &= ~0x01;
          frame_ticks++;

  #ifdef PC_BUILD
        printf("frame update (%x), %d instructions total, %d RAM flushes\n",
           reg[REG_PC], instruction_count - last_frame, flush_ram_count);
          last_frame = instruction_count;

/*          printf("%d gbc audio updates\n", gbc_update_count);
          printf("%d oam updates\n", oam_update_count); */
          gbc_update_count = 0;
          oam_update_count = 0;
          flush_ram_count = 0;
  #endif

          if(update_input())
            continue;

          update_gbc_sound(cpu_ticks);

          if(gp2x_fps_debug)
          {
            char print_buffer[32];
            sprintf(print_buffer, "%d (%d)", (int)fps, (int)frames_drawn);
            print_string(print_buffer, 0xFFFF, 0x000, 0, 0);
          }

          update_screen();

          synchronize();

          if(update_backup_flag)
            update_backup();

          process_cheats();

          event_cycles++;
          if(event_cycles == event_cycles_trigger)
          {
            trigger_ext_event();
            continue;
          }

          vcount = 0;
        }

        if(vcount == (dispstat >> 8))
        {
          // vcount trigger
          dispstat |= 0x04;
          if(dispstat & 0x20)
          {
            irq_raised |= IRQ_VCOUNT;
          }
        }
        else
        {
          dispstat &= ~0x04;
        }

        io_registers[REG_VCOUNT] = vcount;
#ifdef NSPIRE_BUILD
        if(*(unsigned volatile*)0xC0000020 & 4)
		{
		  *(unsigned volatile*)0xC0000028 = 4;
		  set_display_buffer(nspire_displayed_screen);
		  relative_frame_count++;
		}
#endif
      }
      io_registers[REG_DISPSTAT] = dispstat;
    }

    if(irq_raised)
      raise_interrupt(irq_raised);

    execute_cycles = video_count;

    check_timer(0);
    check_timer(1);
    check_timer(2);
    check_timer(3);
  } while(reg[CPU_HALT_STATE] != CPU_ACTIVE);

  return execute_cycles;
}

u64 last_screen_timestamp = 0;
u32 frame_speed = 15000;


#ifdef PSP_BUILD

u32 real_frame_count = 0;
u32 virtual_frame_count = 0;
u32 num_skipped_frames = 0;

void vblank_interrupt_handler(u32 sub, u32 *parg)
{
  real_frame_count++;
}

void synchronize()
{
  char char_buffer[64];
  u64 new_ticks, time_delta;
  s32 used_frameskip = frameskip_value;

  if(!synchronize_flag)
  {
    print_string("--FF--", 0xFFFF, 0x000, 0, 0);
    used_frameskip = 4;
    virtual_frame_count = real_frame_count - 1;
  }

  skip_next_frame = 0;

  virtual_frame_count++;

  if(real_frame_count >= virtual_frame_count)
  {
    if((real_frame_count > virtual_frame_count) &&
     (current_frameskip_type == auto_frameskip) &&
     (num_skipped_frames < frameskip_value))
    {
      skip_next_frame = 1;
      num_skipped_frames++;
    }
    else
    {
      virtual_frame_count = real_frame_count;
      num_skipped_frames = 0;
    }

    // Here so that the home button return will eventually work.
    // If it's not running fullspeed anyway this won't really hurt
    // it much more.

    delay_us(1);
  }
  else
  {
    if(synchronize_flag)
      sceDisplayWaitVblankStart();
  }

  if(current_frameskip_type == manual_frameskip)
  {
    frameskip_counter = (frameskip_counter + 1) %
     (used_frameskip + 1);
    if(random_skip)
    {
      if(frameskip_counter != (rand() % (used_frameskip + 1)))
        skip_next_frame = 1;
    }
    else
    {
      if(frameskip_counter)
        skip_next_frame = 1;
    }
  }

/*  sprintf(char_buffer, "%08d %08d %d %d %d\n",
   real_frame_count, virtual_frame_count, num_skipped_frames,
   real_frame_count - virtual_frame_count, skip_next_frame);
  print_string(char_buffer, 0xFFFF, 0x0000, 0, 10); */

/*
    sprintf(char_buffer, "%02d %02d %06d %07d", frameskip, (u32)ms_needed,
     ram_translation_ptr - ram_translation_cache, rom_translation_ptr -
     rom_translation_cache);
    print_string(char_buffer, 0xFFFF, 0x0000, 0, 0);
*/
}

#endif

#ifdef GP2X_BUILD

u32 real_frame_count = 0;
u32 virtual_frame_count = 0;
u32 num_skipped_frames = 0;
u32 interval_skipped_frames;
u32 frames;

const u32 frame_interval = 60;

void synchronize()
{
  u64 new_ticks;
  u64 time_delta;

  get_ticks_us(&new_ticks);
  time_delta = new_ticks - last_screen_timestamp;
  last_screen_timestamp = new_ticks;

  skip_next_frame = 0;
  virtual_frame_count++;

  real_frame_count = ((new_ticks -
    frame_count_initial_timestamp) * 3) / 50000;

  if(real_frame_count >= virtual_frame_count)
  {
    if((real_frame_count > virtual_frame_count) &&
     (current_frameskip_type == auto_frameskip) &&
     (num_skipped_frames < frameskip_value))
    {
      skip_next_frame = 1;
      num_skipped_frames++;
    }
    else
    {
      virtual_frame_count = real_frame_count;
      num_skipped_frames = 0;
    }
  }

  frames++;

  if(frames == frame_interval)
  {
    u32 new_fps;
    u32 new_frames_drawn;

    time_delta = new_ticks - last_frame_interval_timestamp;
    new_fps = (u64)((u64)1000000 * (u64)frame_interval) / time_delta;
    new_frames_drawn =
     (frame_interval - interval_skipped_frames) * (60 / frame_interval);

    // Left open for rolling averages
    fps = new_fps;
    frames_drawn = new_frames_drawn;

    last_frame_interval_timestamp = new_ticks;
    interval_skipped_frames = 0;
    frames = 0;
  }

  if(current_frameskip_type == manual_frameskip)
  {
    frameskip_counter = (frameskip_counter + 1) %
     (frameskip_value + 1);
    if(random_skip)
    {
      if(frameskip_counter != (rand() % (frameskip_value + 1)))
        skip_next_frame = 1;
    }
    else
    {
      if(frameskip_counter)
        skip_next_frame = 1;
    }
  }

  interval_skipped_frames += skip_next_frame;

  if(!synchronize_flag)
    print_string("--FF--", 0xFFFF, 0x000, 0, 0);
}

#endif


#ifdef PC_BUILD

u32 ticks_needed_total = 0;
float us_needed = 0.0;
u32 frames = 0;
const u32 frame_interval = 60;

void synchronize()
{
  u64 new_ticks;
  u64 time_delta;
  char char_buffer[64];

  get_ticks_us(&new_ticks);
  time_delta = new_ticks - last_screen_timestamp;
  last_screen_timestamp = new_ticks;
  ticks_needed_total += time_delta;

  skip_next_frame = 0;

  if((time_delta < frame_speed) && synchronize_flag)
  {
    delay_us(frame_speed - time_delta);
  }

  frames++;

  if(frames == frame_interval)
  {
    us_needed = (float)ticks_needed_total / frame_interval;
    ticks_needed_total = 0;
    frames = 0;
  }

  if(current_frameskip_type == manual_frameskip)
  {
    frameskip_counter = (frameskip_counter + 1) %
     (frameskip_value + 1);
    if(random_skip)
    {
      if(frameskip_counter != (rand() % (frameskip_value + 1)))
        skip_next_frame = 1;
    }
    else
    {
      if(frameskip_counter)
        skip_next_frame = 1;
    }
  }

  if(synchronize_flag == 0)
    print_string("--FF--", 0xFFFF, 0x000, 0, 0);

  sprintf(char_buffer, "gpSP: %.1fms %.1ffps", us_needed / 1000.0,
   1000000.0 / us_needed);
  SDL_WM_SetCaption(char_buffer, "gpSP");

/*
    sprintf(char_buffer, "%02d %02d %06d %07d", frameskip, (u32)ms_needed,
     ram_translation_ptr - ram_translation_cache, rom_translation_ptr -
     rom_translation_cache);
    print_string(char_buffer, 0xFFFF, 0x0000, 0, 0);
*/
}

#endif

#ifdef NSPIRE_BUILD

u32 num_skipped_frames = 0;

void synchronize()
{
  skip_next_frame = 0;
  u32 used_frameskip = frameskip_value;
  
  if(!synchronize_flag)
  {
    print_string("--FF--", 0xFFFF, 0x000, 0, 0);
    used_frameskip = 4;
    relative_frame_count = 2;
  }
  
  relative_frame_count--;
  
  if(relative_frame_count >= 0)
  {
	if((relative_frame_count > 0) && 
	   (current_frameskip_type == auto_frameskip) &&
       (num_skipped_frames < used_frameskip))
    {
      skip_next_frame = 1;
      num_skipped_frames++;
    }
    else
    {
      relative_frame_count = 0;
	  num_skipped_frames = 0;
    }
  }
  else
  {
	if (synchronize_flag)
		while((*(unsigned volatile*)0xC0000020 & 4) == 0) { }
  }
  
  if(current_frameskip_type == manual_frameskip)
  {
    frameskip_counter = (frameskip_counter + 1) %
     (used_frameskip + 1);
    if(random_skip)
    {
      if(frameskip_counter != (rand() % (used_frameskip + 1)))
        skip_next_frame = 1;
    }
    else
    {
      if(frameskip_counter)
        skip_next_frame = 1;
    }
  }
  return;
}
#endif

void quit()
{
  save_romdir();
  
  if(!update_backup_flag)
    update_backup_force();

  sound_exit();

#ifdef REGISTER_USAGE_ANALYZE
  print_register_usage();
#endif

#ifdef PSP_BUILD
  sceKernelExitGame();
#else
#ifndef NSPIRE_BUILD
  SDL_Quit();
#endif

#ifdef GP2X_BUILD
  gp2x_quit();
#endif

#ifdef NSPIRE_BUILD
  free(gamepak_rom);
  free(gamepak_memory_map);
  nspire_restore();
#endif

  exit(0);
#endif
}

void reset_gba()
{
  init_main();
  init_memory();
  init_cpu();
  reset_sound();
}

#ifdef PSP_BUILD

u32 file_length(u8 *filename, s32 dummy)
{
  SceIoStat stats;
  sceIoGetstat(filename, &stats);
  return stats.st_size;
}

void delay_us(u32 us_count)
{
  sceKernelDelayThread(us_count);
}

void get_ticks_us(u64 *tick_return)
{
  u64 ticks;
  sceRtcGetCurrentTick(&ticks);

  *tick_return = (ticks * 1000000) / sceRtcGetTickResolution();
}

#else

u32 file_length(u8 *dummy, FILE *fp)
{
  u32 length;

  fseek(fp, 0, SEEK_END);
  length = ftell(fp);
  fseek(fp, 0, SEEK_SET);

  return length;
}

#ifdef PC_BUILD

void delay_us(u32 us_count)
{
  SDL_Delay(us_count / 1000);
}

void get_ticks_us(u64 *ticks_return)
{
  *ticks_return = (SDL_GetTicks() * 1000);
}

#else

#ifdef NSPIRE_BUILD
void delay_us(u32 us_count)
{
	sleep(us_count / 1000);
}

void get_ticks_us(u64 *ticks_return)
{
	*ticks_return = (u64)GetRTC() * 1000000;
}
#else
void delay_us(u32 us_count)
{
  //usleep(us_count);
  SDL_Delay(us_count / 1000);
}

void get_ticks_us(u64 *ticks_return)
{
  struct timeval current_time;
  gettimeofday(&current_time, NULL);

  *ticks_return =
   (u64)current_time.tv_sec * 1000000 + current_time.tv_usec;
}
#endif

#endif

#endif

void change_ext(u8 *src, u8 *buffer, u8 *extension)
{
  char *dot_position;
  strcpy((char*)buffer, (char*)src);
  dot_position = strrchr((char*)buffer, '.');

#ifdef NSPIRE_BUILD
  if(dot_position)
  {
    *dot_position = '\0';
	dot_position = strrchr((char*)buffer, '.');
	if(dot_position)
	  strcpy(dot_position, (char*)extension);
  }
#else
  if(dot_position)
    strcpy(dot_position, (char*)extension);
#endif
}

#define main_savestate_builder(type)                                          \
void main_##type##_savestate(file_tag_type savestate_file)                    \
{                                                                             \
  file_##type##_variable(savestate_file, cpu_ticks);                          \
  file_##type##_variable(savestate_file, execute_cycles);                     \
  file_##type##_variable(savestate_file, video_count);                        \
  file_##type##_array(savestate_file, timer);                                 \
}                                                                             \

main_savestate_builder(read);
main_savestate_builder(write_mem);


void printout(void *str, u32 val)
{
  printf(str, val);
}

void set_clock_speed()
{
  static u32 clock_speed_old = default_clock_speed;
  if (clock_speed != clock_speed_old)
  {
    printf("about to set CPU clock to %iMHz\n", (int)clock_speed);
  #ifdef PSP_BUILD
    scePowerSetClockFrequency(clock_speed, clock_speed, clock_speed / 2);
  #elif defined(GP2X_BUILD)
    set_FCLK(clock_speed);
  #endif
    clock_speed_old = clock_speed;
  }
}

