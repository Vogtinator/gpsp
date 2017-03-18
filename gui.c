/* gameplaySP
 *
 * Copyright (C) 2006 Exophase <exophase@gmail.com>
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public Licens e as
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

#ifndef _WIN32_WCE

#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>

#endif

#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
#include "gp2x/cpuctrl.h"
#endif

#include "common.h"
#include "font.h"

#define MAX_PATH 1024

// Blatantly stolen and trimmed from MZX (megazeux.sourceforge.net)

#if defined(GP2X_BUILD) || defined(NSPIRE_BUILD)

#define FILE_LIST_ROWS ((int)((SDL_SCREEN_HEIGHT - 40) / FONT_HEIGHT))
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION 260

#else

#define FILE_LIST_ROWS 25
#define FILE_LIST_POSITION 5
#define DIR_LIST_POSITION 360

#endif

#ifdef PSP_BUILD

#define color16(red, green, blue)                                             \
  (blue << 11) | (green << 5) | red                                           \

#elif defined(NSPIRE_BUILD)
#define color16(red, green, blue)                                             \
  ((green & 1) << 15) | (blue << 10) | ((green >> 1) << 5) | red 
  
#else

#define color16(red, green, blue)                                             \
  (red << 11) | (green << 5) | blue                                           \

#endif

#ifdef GP2X_BUILD

#define COLOR_BG            color16(0, 0, 0)

#else

#define COLOR_BG            color16(2, 8, 10)

#endif

#define COLOR_ROM_INFO      color16(22, 36, 26)
#define COLOR_ACTIVE_ITEM   color16(31, 63, 31)
#define COLOR_INACTIVE_ITEM color16(13, 40, 18)
#define COLOR_FRAMESKIP_BAR color16(15, 31, 31)
#define COLOR_HELP_TEXT     color16(16, 40, 24)

#ifdef PSP_BUILD
  u8 *clock_speed_options[] =
  {
    "33MHz", "66MHz", "100MHz", "133MHz", "166MHz", "200MHz", "233MHz",
    "266MHz", "300MHz", "333MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = (clock_speed_number + 1) * 33
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed / 33) - 1
#elif defined(WIZ_BUILD)
  u8 *clock_speed_options[] =
  {
    "300MHz", "333MHz", "366MHz", "400MHz", "433MHz",
    "466MHz", "500MHz", "533MHz", "566MHz", "600MHz",
    "633MHz", "666MHz", "700MHz", "733MHz", "766MHz",
    "800MHz", "833MHz", "866MHz", "900MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = 300 + (clock_speed_number * 3333) / 100
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed - 300) / 33
#elif defined(GP2X_BUILD)
  u8 *clock_speed_options[] =
  {
    "150MHz", "160MHz", "170MHz", "180MHz", "190MHz",
    "200MHz", "210MHz", "220MHz", "230MHz", "240MHz",
    "250MHz", "260MHz", "270MHz", "280MHz", "290MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = 150 + clock_speed_number * 10
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed - 150) / 10
#elif defined(NSPIRE_BUILD)
  u8 *clock_speed_options[] =
  {
    "120MHz", "126MHz", "132MHz", "138MHz", "144MHz",
	"150MHz", "156MHz", "162MHz", "168MHz", "174MHz",
	"180MHz", "186MHz", "192MHz", "198MHz", "204MHz",
	"210MHz", "216MHz", "222MHz", "228MHz", "234MHz",
	"240MHz", "246MHz", "252MHz", "258MHz", "264MHz",
	"270MHz", "276MHz", "282MHz", "288MHz", "294MHz",
	"300MHz"
  };
  #define menu_get_clock_speed() \
    clock_speed = 120 + clock_speed_number * 6
  #define get_clock_speed_number() \
    clock_speed_number = (clock_speed - 120) / 6
#else
  u8 *clock_speed_options[] =
  {
    "0"
  };
  #define menu_get_clock_speed() 0
  #define get_clock_speed_number() 0
#endif


int sort_function(const void *dest_str_ptr, const void *src_str_ptr)
{
  char *dest_str = *((char **)dest_str_ptr);
  char *src_str = *((char **)src_str_ptr);

  if(src_str[0] == '.')
    return 1;

  if(dest_str[0] == '.')
    return -1;

  return strcasecmp(dest_str, src_str);
}

s32 load_file(u8 **wildcards, u8 *result)
{
  DIR *current_dir;
  struct dirent *current_file;
  struct stat file_info;
  u8 current_dir_name[MAX_PATH];
  u8 current_dir_short[81];
  u32 current_dir_length;
  u32 total_filenames_allocated;
  u32 total_dirnames_allocated;
  u8 **file_list;
  u8 **dir_list;
  u32 num_files;
  u32 num_dirs;
  u8 *file_name;
  u32 file_name_length;
  u32 ext_pos = -1;
  u32 chosen_file, chosen_dir;
  u32 dialog_result = 1;
  s32 return_value = 1;
  s32 current_file_selection;
  s32 current_file_scroll_value;
  u32 current_dir_selection;
  u32 current_dir_scroll_value;
  s32 current_file_in_scroll;
  u32 current_dir_in_scroll;
  u32 current_file_number, current_dir_number;
  u32 current_column = 0;
  u32 repeat;
  u32 i;
  gui_action_type gui_action;

  while(return_value == 1)
  {
    current_file_selection = 0;
    current_file_scroll_value = 0;
    current_dir_selection = 0;
    current_dir_scroll_value = 0;
    current_file_in_scroll = 0;
    current_dir_in_scroll = 0;

    total_filenames_allocated = 32;
    total_dirnames_allocated = 32;
    file_list = (u8 **)malloc(sizeof(u8 *) * 32);
    dir_list = (u8 **)malloc(sizeof(u8 *) * 32);
    memset(file_list, 0, sizeof(u8 *) * 32);
    memset(dir_list, 0, sizeof(u8 *) * 32);

    num_files = 0;
    num_dirs = 0;
    chosen_file = 0;
    chosen_dir = 0;

    getcwd(current_dir_name, MAX_PATH);

    current_dir = opendir(current_dir_name);

    do
    {
	  if(current_dir)
        current_file = readdir(current_dir);
      else
        current_file = NULL;

      if(current_file)
      {
        file_name = current_file->d_name;
        file_name_length = strlen(file_name);

        if((stat(file_name, &file_info) >= 0) &&
         ((file_name[0] != '.') || (file_name[1] == '.')))
        {
          if(S_ISDIR(file_info.st_mode))
          {
            dir_list[num_dirs] =
             (u8 *)malloc(file_name_length + 1);

            sprintf(dir_list[num_dirs], "%s", file_name);

            num_dirs++;
          }
          else
          {
			// Must match one of the wildcards, also ignore the .
#ifdef NSPIRE_BUILD
			if(file_name_length >= 8)
			{
			  if(file_name[file_name_length - 8] == '.')
			    ext_pos = file_name_length - 8;
#else
            if(file_name_length >= 4)
            {
              if(file_name[file_name_length - 4] == '.')
                ext_pos = file_name_length - 4;
              else

              if(file_name[file_name_length - 3] == '.')
                ext_pos = file_name_length - 3;
#endif
              else
                ext_pos = 0;

              for(i = 0; wildcards[i] != NULL; i++)
              {
                if(!strcasecmp((file_name + ext_pos),
                 wildcards[i]))
                {
                  file_list[num_files] =
                   (u8 *)malloc(file_name_length + 1);

                  sprintf(file_list[num_files], "%s", file_name);

                  num_files++;
                  break;
                }
              }
            }
          }
        }

        if(num_files == total_filenames_allocated)
        {
          file_list = (u8 **)realloc(file_list, sizeof(u8 *) *
           total_filenames_allocated * 2);
          memset(file_list + total_filenames_allocated, 0,
           sizeof(u8 *) * total_filenames_allocated);
          total_filenames_allocated *= 2;
        }

        if(num_dirs == total_dirnames_allocated)
        {
          dir_list = (u8 **)realloc(dir_list, sizeof(u8 *) *
           total_dirnames_allocated * 2);
          memset(dir_list + total_dirnames_allocated, 0,
           sizeof(u8 *) * total_dirnames_allocated);
          total_dirnames_allocated *= 2;
        }
      }
    } while(current_file);
#ifndef NSPIRE_BUILD
    qsort((void *)file_list, num_files, sizeof(u8 *), sort_function);
    qsort((void *)dir_list, num_dirs, sizeof(u8 *), sort_function);
#endif
    closedir(current_dir);

    current_dir_length = strlen(current_dir_name);

    if(current_dir_length > 80)
    {

#if defined(GP2X_BUILD)
    snprintf(current_dir_short, 80,
     "...%s", current_dir_name + current_dir_length - 77);
#else
    memcpy(current_dir_short, "...", 3);
      memcpy(current_dir_short + 3,
       current_dir_name + current_dir_length - 77, 77);
      current_dir_short[80] = 0;
#endif
    }
    else
    {
#if defined(GP2X_BUILD)
      snprintf(current_dir_short, 80, "%s", current_dir_name);
#else
      memcpy(current_dir_short, current_dir_name,
       current_dir_length + 1);
#endif
    }

    repeat = 1;

    if(num_files == 0)
      current_column = 1;

    clear_screen(COLOR_BG);
  {
    u8 print_buffer[81];

    while(repeat)
    {
      flip_screen();
#ifdef NSPIRE_BUILD
	  update_at_vblank();
#endif
	  
#ifdef NSPIRE_BUILD
	clear_screen(COLOR_BG);
#endif

      print_string(current_dir_short, COLOR_ACTIVE_ITEM, COLOR_BG, 0, 0);
#ifdef GP2X_BUILD
      print_string("Press X to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 220);
#elif defined(NSPIRE_BUILD)
	  print_string("Press menu to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 220);
#else
      print_string("Press X to return to the main menu.",
       COLOR_HELP_TEXT, COLOR_BG, 20, 260);
#endif

      for(i = 0, current_file_number = i + current_file_scroll_value;
       i < FILE_LIST_ROWS; i++, current_file_number++)
      {
        if(current_file_number < num_files)
        {
          if((current_file_number == current_file_selection) &&
           (current_column == 0))
          {
            print_string(file_list[current_file_number], COLOR_ACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(file_list[current_file_number], COLOR_INACTIVE_ITEM,
             COLOR_BG, FILE_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }

      for(i = 0, current_dir_number = i + current_dir_scroll_value;
       i < FILE_LIST_ROWS; i++, current_dir_number++)
      {
        if(current_dir_number < num_dirs)
        {
          if((current_dir_number == current_dir_selection) &&
           (current_column == 1))
          {
            print_string(dir_list[current_dir_number], COLOR_ACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
          else
          {
            print_string(dir_list[current_dir_number], COLOR_INACTIVE_ITEM,
             COLOR_BG, DIR_LIST_POSITION, ((i + 1) * 10));
          }
        }
      }

      gui_action = get_gui_input();

      switch(gui_action)
      {
        case CURSOR_DOWN:
          if(current_column == 0)
          {
            if(current_file_selection < (num_files - 1))
            {
              current_file_selection++;
              if(current_file_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value++;
              }
              else
              {
                current_file_in_scroll++;
              }
            }
            else
            {
              clear_screen(COLOR_BG);
              current_file_selection = 0;
              current_file_scroll_value = 0;
              current_file_in_scroll = 0;
            }
          }
          else
          {
            if(current_dir_selection < (num_dirs - 1))
            {
              current_dir_selection++;
              if(current_dir_in_scroll == (FILE_LIST_ROWS - 1))
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value++;
              }
              else
              {
                current_dir_in_scroll++;
              }
            }
          }

          break;

        case CURSOR_R:
          if (current_column != 0)
            break;
          clear_screen(COLOR_BG);
	  current_file_selection += FILE_LIST_ROWS;
          if (current_file_selection > num_files - 1)
            current_file_selection = num_files - 1;
          current_file_scroll_value = current_file_selection - FILE_LIST_ROWS / 2;
          if (current_file_scroll_value < 0)
          {
            current_file_scroll_value = 0;
            current_file_in_scroll = current_file_selection;
          }
          else
          {
            current_file_in_scroll = FILE_LIST_ROWS / 2;
          }
          break;

        case CURSOR_UP:
          if(current_column == 0)
          {
            if(current_file_selection)
            {
              current_file_selection--;
              if(current_file_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_file_scroll_value--;
              }
              else
              {
                current_file_in_scroll--;
              }
            }
            else
            {
              clear_screen(COLOR_BG);
              current_file_selection = num_files - 1;
              current_file_in_scroll = FILE_LIST_ROWS - 1;
              if (current_file_in_scroll > num_files - 1)
                current_file_in_scroll = num_files - 1;
              current_file_scroll_value = num_files - FILE_LIST_ROWS;
              if (current_file_scroll_value < 0)
                current_file_scroll_value = 0;
            }
          }
          else
          {
            if(current_dir_selection)
            {
              current_dir_selection--;
              if(current_dir_in_scroll == 0)
              {
                clear_screen(COLOR_BG);
                current_dir_scroll_value--;
              }
              else
              {
                current_dir_in_scroll--;
              }
            }
          }
          break;

        case CURSOR_L:
          if (current_column != 0)
            break;
          clear_screen(COLOR_BG);
	  current_file_selection -= FILE_LIST_ROWS;
          if (current_file_selection < 0)
            current_file_selection = 0;
          current_file_scroll_value = current_file_selection - FILE_LIST_ROWS / 2;
          if (current_file_scroll_value < 0)
          {
            current_file_scroll_value = 0;
            current_file_in_scroll = current_file_selection;
          }
          else
          {
            current_file_in_scroll = FILE_LIST_ROWS / 2;
          }
          break;

         case CURSOR_RIGHT:
          if(current_column == 0)
          {
            if(num_dirs != 0)
              current_column = 1;
          }
          break;

        case CURSOR_LEFT:
          if(current_column == 1)
          {
            if(num_files != 0)
              current_column = 0;
          }
          break;

        case CURSOR_SELECT:
          if(current_column == 1)
          {
            repeat = 0;
            chdir(dir_list[current_dir_selection]);
          }
          else
          {
            if(num_files != 0)
            {
              repeat = 0;
              return_value = 0;
              strcpy(result, file_list[current_file_selection]);
            }
          }
          break;

        case CURSOR_BACK:
#ifdef PSP_BUILD
          if(!strcmp(current_dir_name, "ms0:/PSP"))
            break;
#endif
#ifdef NSPIRE_BUILD
		  //if(!strcmp(current_dir_name, "\\documents\\"))
			//break;
#endif
          repeat = 0;
          chdir("..");
          break;

        case CURSOR_EXIT:
          return_value = -1;
          repeat = 0;
          break;
      }
    }
  }

    for(i = 0; i < num_files; i++)
    {
      free(file_list[i]);
    }
    free(file_list);

    for(i = 0; i < num_dirs; i++)
    {
      free(dir_list[i]);
    }
    free(dir_list);
  }

  clear_screen(COLOR_BG);

  return return_value;
}

typedef enum
{
  NUMBER_SELECTION_OPTION = 0x01,
  STRING_SELECTION_OPTION = 0x02,
  SUBMENU_OPTION          = 0x04,
  ACTION_OPTION           = 0x08,
  KEY_SELECTION_OPTION    = 0x10
} menu_option_type_enum;

struct _menu_type
{
  void (* init_function)();
  void (* passive_function)();
  struct _menu_option_type *options;
  u32 num_options;
};

struct _menu_option_type
{
  void (* action_function)();
  void (* passive_function)();
  struct _menu_type *sub_menu;
  char *display_string;
  void *options;
  u32 *current_option;
  u32 num_options;
  char *help_string;
  u32 line_number;
  menu_option_type_enum option_type;
};

typedef struct _menu_option_type menu_option_type;
typedef struct _menu_type menu_type;

#define make_menu(name, init_function, passive_function)                      \
  menu_type name##_menu =                                                     \
  {                                                                           \
    init_function,                                                            \
    passive_function,                                                         \
    name##_options,                                                           \
    sizeof(name##_options) / sizeof(menu_option_type)                         \
  }                                                                           \

#define gamepad_config_option(display_string, number)                         \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + gamepad_config_line_to_button[number],                 \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[                                            \
   gamepad_config_line_to_button[number]]],                                   \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define analog_config_option(display_string, number)                          \
{                                                                             \
  NULL,                                                                       \
  menu_fix_gamepad_help,                                                      \
  NULL,                                                                       \
  display_string ": %s",                                                      \
  gamepad_config_buttons,                                                     \
  gamepad_config_map + number + 12,                                           \
  sizeof(gamepad_config_buttons) / sizeof(gamepad_config_buttons[0]),         \
  gamepad_help[gamepad_config_map[number + 12]],                              \
  number + 2,                                                                 \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#ifdef NSPIRE_BUILD
#define key_config_option(number, display_string, help)                       \
{                                                                             \
  choose_key,                                                                 \
  NULL,                                                                       \
  NULL,                                                                       \
  display_string,                                                             \
  nspire_key_strings,                                                         \
  gamepad_config_map + number,                                                \
  0,                                                                          \
  help,                                                                       \
  number-1,                                                                   \
  ACTION_OPTION                                                               \
} 
#endif
  
#define cheat_option(number)                                                  \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  NULL,                                                                       \
  cheat_format_str[number],                                                   \
  enable_disable_options,                                                     \
  &(cheats[number].cheat_active),                                             \
  2,                                                                          \
  "Activate/deactivate this cheat code.",                                     \
  number,                                                                     \
  STRING_SELECTION_OPTION                                                     \
}                                                                             \

#define action_option(action_function, passive_function, display_string,      \
 help_string, line_number)                                                    \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  0,                                                                          \
  help_string,                                                                \
  line_number,                                                                \
  ACTION_OPTION                                                               \
}                                                                             \

#define submenu_option(sub_menu, display_string, help_string, line_number)    \
{                                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sub_menu,                                                                   \
  display_string,                                                             \
  NULL,                                                                       \
  NULL,                                                                       \
  sizeof(sub_menu) / sizeof(menu_option_type),                                \
  help_string,                                                                \
  line_number,                                                                \
  SUBMENU_OPTION                                                              \
}                                                                             \

#define selection_option(passive_function, display_string, options,           \
 option_ptr, num_options, help_string, line_number, type)                     \
{                                                                             \
  NULL,                                                                       \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type                                                                        \
}                                                                             \

#define action_selection_option(action_function, passive_function,            \
 display_string, options, option_ptr, num_options, help_string, line_number,  \
 type)                                                                        \
{                                                                             \
  action_function,                                                            \
  passive_function,                                                           \
  NULL,                                                                       \
  display_string,                                                             \
  options,                                                                    \
  option_ptr,                                                                 \
  num_options,                                                                \
  help_string,                                                                \
  line_number,                                                                \
  type | ACTION_OPTION                                                        \
}                                                                             \


#define string_selection_option(passive_function, display_string, options,    \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %s", options,          \
   option_ptr, num_options, help_string, line_number, STRING_SELECTION_OPTION)\

#define numeric_selection_option(passive_function, display_string,            \
 option_ptr, num_options, help_string, line_number)                           \
  selection_option(passive_function, display_string ": %d", NULL, option_ptr, \
   num_options, help_string, line_number, NUMBER_SELECTION_OPTION)            \

#define string_selection_(action_function, passive_function,     \
 display_string, options, option_ptr, num_options, help_string, line_number)  \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %s",  options, option_ptr, num_options, help_string,     \
   line_number, STRING_SELECTION_OPTION)                                      \

#define numeric_selection_action_option(action_function, passive_function,    \
 display_string, option_ptr, num_options, help_string, line_number)           \
  action_selection_option(action_function, passive_function,                  \
   display_string ": %d",  NULL, option_ptr, num_options, help_string,        \
   line_number, NUMBER_SELECTION_OPTION)                                      \

#define numeric_selection_action_hide_option(action_function,                 \
 passive_function, display_string, option_ptr, num_options, help_string,      \
 line_number)                                                                 \
  action_selection_option(action_function, passive_function,                  \
   display_string, NULL, option_ptr, num_options, help_string,                \
   line_number, NUMBER_SELECTION_OPTION)                                      \

#define GAMEPAD_MENU_WIDTH 15

#ifdef PSP_BUILD

u32 gamepad_config_line_to_button[] =
 { 8, 6, 7, 9, 1, 2, 3, 0, 4, 5, 11, 10 };

#endif

#ifdef GP2X_BUILD

u32 gamepad_config_line_to_button[] =
 { 0, 2, 1, 3, 8, 9, 10, 11, 6, 7, 4, 5, 14 };

#endif

#ifdef NSPIRE_BUILD

u32 gamepad_config_line_to_button[] =
 { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 };

char *nspire_key_strings[] =
{
    "NONE",
	"return", "enter", NULL, "(-)", "space", "Z", "Y", "0", "?!", "home", NULL, NULL, NULL, NULL, NULL, NULL,
    "X", "W", "V", "3", "U", "T", "S", "1", "pi", "trig", "10^x", NULL, NULL, NULL, NULL, NULL,
    "R", "Q", "P", "6", "O", "N", "M", "4", "EE", "x^2", NULL, NULL, NULL, NULL, NULL, NULL,
    "L", "K", "J", "9", "I", "H", "G", "7", "/", "e^x", NULL, NULL, NULL, NULL, NULL, NULL,
    "F", "E", "D", NULL, "C", "B", "A", "=", "*", "^", NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, "var", "-", ")", ".", "(", "5", "cat", "frac", "del", "scratch", NULL, NULL, NULL, NULL, NULL,
    "flag", "click", "+", "doc", "2", "menu", "8", "esc", NULL, "tab", NULL, NULL, NULL, NULL, NULL, NULL,
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "shift", "ctrl", ",", NULL, NULL, NULL, NULL, NULL
};
 
#endif

u8 *scale_options[] =
{
#ifdef WIZ_BUILD
  "unscaled 3:2", "scaled 3:2 (slower)",
  "unscaled 3:2 (anti-tear)", "scaled 3:2 (anti-tear)"
#elif NSPIRE_BUILD
  "unscaled 3:2 (fastest)", "scaled/blended 3:2 (slowest)", "fullscreen/blended (slower)", "scaled/unblended 3:2 (fast)"
#else
  "unscaled 3:2", "scaled 3:2", "fullscreen", "scaled 3:2 (software)"
#ifdef PSP_BUILD
  " 16:9"
#endif
#endif
};

s32 load_game_config_file()
{
  u8 game_config_filename[512];
  u32 file_loaded = 0;
  u32 i;
#ifdef NSPIRE_BUILD
  change_ext(gamepak_filename, game_config_filename, ".cfg.tns");
#else
  change_ext(gamepak_filename, game_config_filename, ".cfg");
#endif

  file_open(game_config_file, game_config_filename, read);

  if(file_check_valid(game_config_file))
  {
    u32 file_size = file_length(game_config_filename, game_config_file);

    // Sanity check: File size must be the right size
    if(file_size == 60)
    {
      u32 file_options[file_size / 4];

      file_read_array(game_config_file, file_options);
      current_frameskip_type = file_options[0] % 3;
      frameskip_value = file_options[1];
      random_skip = file_options[2] % 2;
      clock_speed = file_options[3];

#ifdef WIZ_BUILD
      if(clock_speed > 900)
        clock_speed = 533;
#elif defined(GP2X_BUILD)
      if(clock_speed >= 300)
        clock_speed = 200;
#elif defined(PSP_BUILD)
      if(clock_speed > 333)
        clock_speed = 333;
#elif defined(NSPIRE_BUILD)
	  if(clock_speed > 300)
	    clock_speed = 132;
#endif

      if(clock_speed < 33)
        clock_speed = 33;

      if(frameskip_value < 0)
        frameskip_value = 0;

      if(frameskip_value > 99)
        frameskip_value = 99;

      for(i = 0; i < 10; i++)
      {
        cheats[i].cheat_active = file_options[4 + i] % 2;
        cheats[i].cheat_name[0] = 0;
      }
      
	  quick_save_slot = file_options[14];
	  
      file_loaded = 1;
    }
	file_close(game_config_file);
  }

  if(file_loaded)
    return 0;

  current_frameskip_type = auto_frameskip;
  frameskip_value = 4;
#ifdef WIZ_BUILD
  frameskip_value = 1;
#endif
  random_skip = 0;
  clock_speed = default_clock_speed;

  for(i = 0; i < 10; i++)
  {
    cheats[i].cheat_active = 0;
    cheats[i].cheat_name[0] = 0;
  }

  return -1;
}

s32 load_config_file()
{
  u8 config_path[512];

  #if (defined(PSP_BUILD) || defined(ARM_ARCH)) && !defined(_WIN32_WCE)
    sprintf(config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);
  #else
    sprintf(config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
  #endif

  file_open(config_file, config_path, read);

  if(file_check_valid(config_file))
  {
    u32 file_size = file_length(config_path, config_file);

    // Sanity check: File size must be the right size
    if(file_size == 92)
    {
      u32 file_options[file_size / 4];
      u32 i;
      s32 menu_button = -1;
      file_read_array(config_file, file_options);

      screen_scale = file_options[0] %
        (sizeof(scale_options) / sizeof(scale_options[0]));
      screen_filter = file_options[1] % 2;
      global_enable_audio = file_options[2] % 2;

#ifdef PSP_BUILD
      audio_buffer_size_number = file_options[3] % 10;
#else
      audio_buffer_size_number = file_options[3] % 11;
#endif

      update_backup_flag = file_options[4] % 2;
      global_enable_analog = file_options[5] % 2;
      analog_sensitivity_level = file_options[6] % 8;

#ifdef PSP_BUILD
    scePowerSetClockFrequency(clock_speed, clock_speed, clock_speed / 2);
#endif

      // Sanity check: Make sure there's a MENU or FRAMESKIP
      // key, if not assign to triangle

#ifndef PC_BUILD
#ifdef NSPIRE_BUILD
	  for(i = 0; i < 16; i++)
	  {
	    gamepad_config_map[i] = file_options[7 + i];
		if(gamepad_config_map[i] > 128 || nspire_key_strings[gamepad_config_map[i]] == NULL)
			gamepad_config_map[i] = 0;
	  }
	  if(!gamepad_config_map[10]) //KEY_MAP_MENU
		gamepad_config_map[10] = 102; //KEY_MENU
#else
      for(i = 0; i < 16; i++)
      {
        gamepad_config_map[i] = file_options[7 + i] %
         (BUTTON_ID_NONE + 1);

        if(gamepad_config_map[i] == BUTTON_ID_MENU)
        {
          menu_button = i;
        }
      }

      if(menu_button == -1)
      {
        gamepad_config_map[0] = BUTTON_ID_MENU;
      }
#endif
#endif

      file_close(config_file);
    }

    return 0;
  }

  return -1;
}

s32 save_game_config_file()
{
  u8 game_config_filename[512];
  u32 i;

#ifdef NSPIRE_BUILD
  change_ext(gamepak_filename, game_config_filename, ".cfg.tns");
#else
  change_ext(gamepak_filename, game_config_filename, ".cfg");
#endif

  file_open(game_config_file, game_config_filename, write);

  if(file_check_valid(game_config_file))
  {
    u32 file_options[15];

    file_options[0] = current_frameskip_type;
    file_options[1] = frameskip_value;
    file_options[2] = random_skip;
    file_options[3] = clock_speed;

    for(i = 0; i < 10; i++)
    {
      file_options[4 + i] = cheats[i].cheat_active;
    }
	
	file_options[14] = quick_save_slot;
	
    file_write_array(game_config_file, file_options);

    file_close(game_config_file);

    return 0;
  }

  return -1;
}

s32 save_config_file()
{
  u8 config_path[512];

  #if (defined(PSP_BUILD) || defined(ARM_ARCH)) && !defined(_WIN32_WCE)
    sprintf(config_path, "%s/%s", main_path, GPSP_CONFIG_FILENAME);
  #else
    sprintf(config_path, "%s\\%s", main_path, GPSP_CONFIG_FILENAME);
  #endif

  save_game_config_file();
  
  file_open(config_file, config_path, write);

  if(file_check_valid(config_file))
  {
    u32 file_options[23];
    u32 i;

    file_options[0] = screen_scale;
    file_options[1] = screen_filter;
    file_options[2] = global_enable_audio;
    file_options[3] = audio_buffer_size_number;
    file_options[4] = update_backup_flag;
    file_options[5] = global_enable_analog;
    file_options[6] = analog_sensitivity_level;

#ifndef PC_BUILD
    for(i = 0; i < 16; i++)
    {
      file_options[7 + i] = gamepad_config_map[i];
    }
#endif

    file_write_array(config_file, file_options);

    file_close(config_file);

    return 0;
  }

  return -1;
}

typedef enum
{
  MAIN_MENU,
  GAMEPAD_MENU,
  SAVESTATE_MENU,
  FRAMESKIP_MENU,
  CHEAT_MENU
} menu_enum;

u32 savestate_slot = 0;

void get_savestate_snapshot(u8 *savestate_filename)
{
  static u16 snapshot_buffer[240 * 160];
  u8 savestate_timestamp_string[80];

  file_open(savestate_file, savestate_filename, read);

  if(file_check_valid(savestate_file))
  {
    u8 weekday_strings[7][11] =
    {
      "Sunday", "Monday", "Tuesday", "Wednesday",
      "Thursday", "Friday", "Saturday"
    };
    time_t savestate_time_flat;
    struct tm *current_time;
    file_read_array(savestate_file, snapshot_buffer);
    file_read_variable(savestate_file, savestate_time_flat);

    file_close(savestate_file);

    current_time = localtime(&savestate_time_flat);
    sprintf(savestate_timestamp_string,
     "%s  %02d/%02d/%04d  %02d:%02d:%02d                ",
     weekday_strings[current_time->tm_wday], current_time->tm_mon + 1,
     current_time->tm_mday, current_time->tm_year + 1900,
     current_time->tm_hour, current_time->tm_min, current_time->tm_sec);

    savestate_timestamp_string[40] = 0;
    print_string(savestate_timestamp_string, COLOR_HELP_TEXT, COLOR_BG,
     10, 40);
  }
  else
  {
    memset(snapshot_buffer, 0, 240 * 160 * 2);
    print_string_ext("No savestate exists for this slot.",
     0xFFFF, 0x0000, 15, 75, snapshot_buffer, 240, 0, 0, FONT_HEIGHT);
    print_string("---------- --/--/---- --:--:--          ", COLOR_HELP_TEXT,
     COLOR_BG, 10, 40);
  }

#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
  blit_to_screen(snapshot_buffer, 240, 160, 230, 40);
#endif
}

void get_savestate_filename(u32 slot, u8 *name_buffer)
{
  u8 savestate_ext[16];

#ifdef NSPIRE_BUILD
  sprintf(savestate_ext, "%d.svs.tns", slot);
#else
  sprintf(savestate_ext, "%d.svs", slot);
#endif
  change_ext(gamepak_filename, name_buffer, savestate_ext);

  get_savestate_snapshot(name_buffer);
}

void get_savestate_filename_noshot(u32 slot, u8 *name_buffer)
{
  u8 savestate_ext[16];
#ifdef NSPIRE_BUILD
  sprintf(savestate_ext, "%d.svs.tns", slot);
#else
  sprintf(savestate_ext, "%d.svs", slot);
#endif
  change_ext(gamepak_filename, name_buffer, savestate_ext);
}

#ifdef PSP_BUILD
  void _flush_cache()
  {
    invalidate_all_cache();
  }
#endif

u32 menu(u16 *original_screen)
{
  u8 print_buffer[81];
  u32 clock_speed_number;
  u32 _current_option = 0;
  gui_action_type gui_action;
  menu_enum _current_menu = MAIN_MENU;
  u32 i;
  u32 repeat = 1;
  u32 return_value = 0;
  u32 first_load = 0;
  u8 savestate_ext[16];
  u8 current_savestate_filename[512];
  u8 line_buffer[80];
  u8 cheat_format_str[10][41];

  menu_type *current_menu;
  menu_option_type *current_option;
  menu_option_type *display_option;
  u32 current_option_num;

  auto void choose_menu();
  auto void clear_help();

#ifdef NSPIRE_BUILD
  void show_key()
  {
    strcpy(&current_option->display_string[14], nspire_key_strings[*current_option->current_option]);
  }
  
  void choose_key()
  {
	print_string("<press a key>", COLOR_ACTIVE_ITEM, COLOR_BG, 10 + 14*FONT_WIDTH, current_option->line_number * 10 + 40);
	flip_screen();
	update_at_vblank();
	u32 key_pressed = key_scan();
	//print_string("             ", COLOR_BG, COLOR_BG, 10 + 14*FONT_WIDTH, current_option->line_number * 10 + 40);
	
	if (key_pressed <= 128 && nspire_key_strings[key_pressed])
		*current_option->current_option = key_pressed;
	else
		*current_option->current_option = 0;
	show_key();
  };
  
  menu_option_type gamepad_config_options[] =
  {
    key_config_option( 0, "D-pad up             ", "Up button on GBA d-pad."),
    key_config_option( 1, "D-pad down           ", "Down button on GBA d-pad."),
    key_config_option( 2, "D-pad left           ", "Left button on GBA d-pad."),
    key_config_option( 3, "D-pad right          ", "Right button on GBA d-pad."),
    key_config_option( 4, "A                    ", "A button on GBA."),
    key_config_option( 5, "B                    ", "B button on GBA."),
    key_config_option( 6, "Left Trigger         ", "Left shoulder button on GBA."),
    key_config_option( 7, "Right Trigger        ", "Right shoulder button on GBA."),
    key_config_option( 8, "Start                ", "Start button on GBA."),
    key_config_option( 9, "Select               ", "Select button on GBA."),
    key_config_option(10, "Menu                 ", "Brings up the options menu."),
	key_config_option(11, "Fast Forward         ", "Toggles fastforward on/off."),
	key_config_option(12, "Load State           ", "Loads the game state from the current slot."),
	key_config_option(13, "Save State           ", "Saves the game state to the current slot."),
	key_config_option(14, "Save+Exit            ", "Saves a state and exits; will be auto-loaded later."),
	
    submenu_option(NULL, "Back", "Return to the main menu.", 15)
  };


  menu_option_type analog_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };
#endif
  
  u8 *gamepad_help[] =
  {
    "Up button on GBA d-pad.",
    "Down button on GBA d-pad.",
    "Left button on GBA d-pad.",
    "Right button on GBA d-pad.",
    "A button on GBA.",
    "B button on GBA.",
    "Left shoulder button on GBA.",
    "Right shoulder button on GBA.",
    "Start button on GBA.",
    "Select button on GBA.",
    "Brings up the options menu.",
    "Toggles fastforward on/off.",
    "Loads the game state from the current slot.",
    "Saves the game state to the current slot.",
    "Rapidly press/release the A button on GBA.",
    "Rapidly press/release the B button on GBA.",
    "Rapidly press/release the L shoulder on GBA.",
    "Rapidly press/release the R shoulder on GBA.",
    "Increases the volume.",
    "Decreases the volume.",
    "Displays virtual/drawn frames per second.",
    "Does nothing."
  };

  void menu_update_clock()
  {
    get_clock_speed_number();
    if (clock_speed_number < 0 || clock_speed_number >=
     sizeof(clock_speed_options) / sizeof(clock_speed_options[0]))
    {
      clock_speed = default_clock_speed;
      get_clock_speed_number();
    }
  }

  void menu_exit()
  {
    if(!first_load)
     repeat = 0;
  }

  void menu_quit()
  {
    menu_get_clock_speed();
    save_config_file();
    quit();
  }

  void menu_load()
  {
#ifdef NSPIRE_BUILD
	u8 *file_ext[] = { ".gba.tns", ".zip.tns", NULL };
#else
	u8 *file_ext[] = { ".gba", ".bin", ".zip", NULL };
#endif
    u8 load_filename[512];
    save_game_config_file();
    if(load_file(file_ext, load_filename) != -1)
    {
       if(load_gamepak(load_filename) == -1)
       {
         quit();
       }
       reset_gba();
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
       return_value = 1;
       repeat = 0;
       reg[CHANGED_PC_STATUS] = 1;
       menu_update_clock();
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  void menu_restart()
  {
    if(!first_load)
    {
      reset_gba();
      reg[CHANGED_PC_STATUS] = 1;
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_change_state()
  {
    get_savestate_filename(savestate_slot, current_savestate_filename);
  }

  void menu_save_state()
  {
    if(!first_load)
    {
      get_savestate_filename_noshot(savestate_slot,
       current_savestate_filename);
      save_state(current_savestate_filename, original_screen);
    }
    menu_change_state();
  }

  void menu_load_state()
  {
    if(!first_load)
    {
      load_state(current_savestate_filename);
      return_value = 1;
      repeat = 0;
    }
  }

  void menu_load_state_file()
  {
#ifdef NSPIRE_BUILD
    u8 *file_ext[] = { ".svs.tns", NULL };
#else
    u8 *file_ext[] = { ".svs", NULL };
#endif
    u8 load_filename[512];
    if(load_file(file_ext, load_filename) != -1)
    {
      load_state(load_filename);
      return_value = 1;
      repeat = 0;
    }
    else
    {
      choose_menu(current_menu);
    }
  }

  void menu_fix_gamepad_help()
  {
#ifndef PC_BUILD
    clear_help();
    current_option->help_string =
     gamepad_help[gamepad_config_map[
     gamepad_config_line_to_button[current_option_num]]];
#endif
  }

  void submenu_graphics_sound()
  {

  }

  void submenu_cheats_misc()
  {

  }

  void submenu_gamepad()
  {
#ifdef NSPIRE_BUILD
	for(i = 0; i < 15; i++)
	{
	  strcpy(&gamepad_config_options[i].display_string[14], nspire_key_strings[*gamepad_config_options[i].current_option]);
	}
#endif
  }

  void submenu_analog()
  {

  }

  void submenu_savestate()
  {
    print_string("Savestate options:", COLOR_ACTIVE_ITEM, COLOR_BG, 10, 70);
    menu_change_state();
  }

  void submenu_main()
  {
    if (*gamepak_filename)
	{
		strncpy(print_buffer, gamepak_filename, 80);
		print_string(print_buffer, COLOR_ROM_INFO, COLOR_BG, 10, 10);
		sprintf(print_buffer, "%s  %s  %s", gamepak_title,
		 gamepak_code, gamepak_maker);
		print_string(print_buffer, COLOR_ROM_INFO, COLOR_BG, 10, 20);

		get_savestate_filename_noshot(savestate_slot,
		 current_savestate_filename);
	}
	else
	{
		print_string("gpSP-Nspire Alpha v0.11", COLOR_ACTIVE_ITEM, COLOR_BG, 10, 10);
	}
  }

  u8 *yes_no_options[] = { "no", "yes" };
  u8 *enable_disable_options[] = { "disabled", "enabled" };

  u8 *frameskip_options[] = { "automatic", "manual", "off" };
  u8 *frameskip_variation_options[] = { "uniform", "random" };

#ifndef PSP_BUILD
  u8 *audio_buffer_options[] =
  {
    "16 bytes", "32 bytes", "64 bytes",
    "128 bytes", "256 bytes", "512 bytes", "1024 bytes", "2048 bytes",
    "4096 bytes", "8192 bytes", "16284 bytes"
  };
#else
  u8 *audio_buffer_options[] =
  {
    "3072 bytes", "4096 bytes", "5120 bytes", "6144 bytes", "7168 bytes",
    "8192 bytes", "9216 bytes", "10240 bytes", "11264 bytes", "12288 bytes"
  };

#endif

  u8 *update_backup_options[] = { "Exit only", "Automatic" };

  u8 *gamepad_config_buttons[] =
  {
    "UP",
    "DOWN",
    "LEFT",
    "RIGHT",
    "A",
    "B",
    "L",
    "R",
    "START",
    "SELECT",
    "MENU",
    "FASTFORWARD",
    "LOAD STATE",
    "SAVE STATE",
    "RAPIDFIRE A",
    "RAPIDFIRE B",
    "RAPIDFIRE L",
    "RAPIDFIRE R",
    "VOLUME UP",
    "VOLUME DOWN",
    "DISPLAY FPS",
    "NOTHING"
  };

  // Marker for help information, don't go past this mark (except \n)------*
  menu_option_type graphics_sound_options[] =
  {
    string_selection_option(NULL, "Display scaling", scale_options,
     (u32 *)(&screen_scale),
     sizeof(scale_options) / sizeof(scale_options[0]),
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
     "Determines how the GBA screen is resized in relation to the entire\n"
     "screen. Select unscaled 3:2 for GBA resolution, scaled 3:2 for GBA\n"
     "aspect ratio scaled to fill the height of the PSP screen, and\n"
     "fullscreen to fill the entire PSP screen."
#endif
     "", 2),
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
    string_selection_option(NULL, "Screen filtering", yes_no_options,
     (u32 *)(&screen_filter), 2,
     "Determines whether or not bilinear filtering should be used when\n"
     "scaling the screen. Selecting this will produce a more even and\n"
     "smooth image, at the cost of being blurry and having less vibrant\n"
     "colors.", 3),
#endif
    string_selection_option(NULL, "Frameskip type", frameskip_options,
     (u32 *)(&current_frameskip_type), 3,
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
     "Determines what kind of frameskipping to use.\n"
     "Frameskipping may improve emulation speed of many games.\n"
#endif
     "Off: Do not skip any frames.\n"
     "Auto: Skip up to N frames (see next opt) as needed.\n"
     "Manual: Always render only 1 out of N + 1 frames."
     , 5),
    numeric_selection_option(NULL, "Frameskip value", &frameskip_value, 100,
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
     "For auto frameskip, determines the maximum number of frames that\n"
     "are allowed to be skipped consecutively.\n"
     "For manual frameskip, determines the number of frames that will\n"
     "always be skipped."
#endif
     "", 6),
    string_selection_option(NULL, "Framskip variation",
     frameskip_variation_options, &random_skip, 2,
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
     "If objects in the game flicker at a regular rate certain manual\n"
     "frameskip values may cause them to normally disappear. Change this\n"
     "value to 'random' to avoid this. Do not use otherwise, as it tends to\n"
     "make the image quality worse, especially in high motion games."
#endif
     "", 7),
#ifndef NSPIRE_BUILD
    string_selection_option(NULL, "Audio output", yes_no_options,
     &global_enable_audio, 2,
     "Select 'no' to turn off all audio output. This will\n"
     "not result in a significant change in performance.", 9),
#ifndef PSP_BUILD
    string_selection_option(NULL, "Audio buffer", audio_buffer_options,
             &audio_buffer_size_number, 11,
#else
    string_selection_option(NULL, "Audio buffer", audio_buffer_options,
             &audio_buffer_size_number, 10,
#endif

#ifdef PSP_BUILD
     "Set the size (in bytes) of the audio buffer. Larger values may result\n"
     "in slightly better performance at the cost of latency; the lowest\n"
     "value will give the most responsive audio.\n"
     "This option requires gpSP to be restarted before it will take effect.",
#else
     "Set the size (in bytes) of the audio buffer.\n"
     "This option requires gpSP restart to take effect.",
#endif
     10),
#endif
    submenu_option(NULL, "Back", "Return to the main menu.", 12)
  };

  make_menu(graphics_sound, submenu_graphics_sound, NULL);

  menu_option_type cheats_misc_options[] =
  {
    cheat_option(0),
    cheat_option(1),
    cheat_option(2),
    cheat_option(3),
    cheat_option(4),
    cheat_option(5),
    cheat_option(6),
    cheat_option(7),
    cheat_option(8),
    cheat_option(9),
#ifndef NSPIRE_BUILD
    string_selection_option(NULL, "Clock speed",
     clock_speed_options, &clock_speed_number,
     sizeof(clock_speed_options) / sizeof(clock_speed_options[0]),
     "Change the clock speed of the device. Higher clock\n"
     "speed will yield better performance, but will drain\n"
     "battery life further.", 11),
#endif
    string_selection_option(NULL, "Update backup",
     update_backup_options, &update_backup_flag, 2,
#ifdef GP2X_BUILD
     "Determines when in-game save files should be\n"
     "written back to SD card.",
#elif defined(NSPIRE_BUILD)
	 "Determines when in-game save files should be\n"
	 "written back to internal storage memory.\n",
#else
     "Determines when in-game save files should be written back to\n"
     "memstick. If set to 'automatic' writebacks will occur shortly after\n"
     "the game's backup is altered. On 'exit only' it will only be written\n"
     "back when you exit from this menu (NOT from using the home button).\n"
     "Use the latter with extreme care.",
#endif
     12),
    submenu_option(NULL, "Back", "Return to the main menu.", 14)
  };

  make_menu(cheats_misc, submenu_cheats_misc, NULL);

  menu_option_type savestate_options[] =
  {
    numeric_selection_action_hide_option(menu_load_state, menu_change_state,
     "Load savestate from current slot", &savestate_slot, 10,
     "Select to load the game state from the current slot\n"
     "for this game.\n"
     "Press left + right to change the current slot.", 6),
    numeric_selection_action_hide_option(menu_save_state, menu_change_state,
     "Save savestate to current slot", &savestate_slot, 10,
     "Select to save the game state to the current slot\n"
     "for this game.\n"
     "Press left + right to change the current slot.", 7),
    numeric_selection_action_hide_option(menu_load_state_file,
      menu_change_state,
     "Load savestate from file", &savestate_slot, 10,
     "Restore gameplay from a savestate file.\n"
     "Note: The same file used to save the state must be\n"
     "present.\n", 9),
    numeric_selection_option(menu_change_state,
     "Current savestate slot", &savestate_slot, 10,
     "Change the current savestate slot.\n", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  make_menu(savestate, submenu_savestate, NULL);

#ifdef PSP_BUILD

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("Circle       ", 4),
    gamepad_config_option("Cross        ", 5),
    gamepad_config_option("Square       ", 6),
    gamepad_config_option("Triangle     ", 7),
    gamepad_config_option("Left Trigger ", 8),
    gamepad_config_option("Right Trigger", 9),
    gamepad_config_option("Start        ", 10),
    gamepad_config_option("Select       ", 11),
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };


  menu_option_type analog_config_options[] =
  {
    analog_config_option("Analog up   ", 0),
    analog_config_option("Analog down ", 1),
    analog_config_option("Analog left ", 2),
    analog_config_option("Analog right", 3),
    string_selection_option(NULL, "Enable analog", yes_no_options,
     &global_enable_analog, 2,
     "Select 'no' to block analog input entirely.", 7),
    numeric_selection_option(NULL, "Analog sensitivity",
     &analog_sensitivity_level, 10,
     "Determine sensitivity/responsiveness of the analog input.\n"
     "Lower numbers are less sensitive.", 8),
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#ifdef GP2X_BUILD

  menu_option_type gamepad_config_options[] =
  {
    gamepad_config_option("D-pad up     ", 0),
    gamepad_config_option("D-pad down   ", 1),
    gamepad_config_option("D-pad left   ", 2),
    gamepad_config_option("D-pad right  ", 3),
    gamepad_config_option("A            ", 4),
    gamepad_config_option("B            ", 5),
    gamepad_config_option("X            ", 6),
    gamepad_config_option("Y            ", 7),
    gamepad_config_option("Left Trigger ", 8),
    gamepad_config_option("Right Trigger", 9),
#ifdef WIZ_BUILD
    gamepad_config_option("Menu         ", 10),
#else
    gamepad_config_option("Start        ", 10),
#endif
    gamepad_config_option("Select       ", 11),
#ifndef WIZ_BUILD
    gamepad_config_option("Stick Push   ", 12),
#endif
    submenu_option(NULL, "Back", "Return to the main menu.", 14)
  };


  menu_option_type analog_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

#ifdef PC_BUILD

  menu_option_type gamepad_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 13)
  };

  menu_option_type analog_config_options[] =
  {
    submenu_option(NULL, "Back", "Return to the main menu.", 11)
  };

#endif

  make_menu(gamepad_config, submenu_gamepad, NULL);
  make_menu(analog_config, submenu_analog, NULL);

  menu_option_type main_options[] =
  {
#ifdef NSPIRE_BUILD
	submenu_option(&graphics_sound_menu, "Graphics options",
     "Select to set display scale size and\n"
     "frameskip behavior.", 0),
#else
    submenu_option(&graphics_sound_menu, "Graphics and Sound options",
     "Select to set display parameters and frameskip\n"
     "behavior, audio on/off, buffer size, and filtering.", 0),
#endif
    numeric_selection_action_option(menu_load_state, NULL,
     "Load state from slot", &savestate_slot, 10,
     "Select to load the game state from the current slot\n"
     "for this game, if it exists.\n"
     "Press left + right to change the current slot.", 2),
    numeric_selection_action_option(menu_save_state, NULL,
     "Save state to slot", &savestate_slot, 10,
     "Select to save the game state to the current slot\n"
     "for this game. See the extended menu for more info.\n"
     "Press left + right to change the current slot.", 3),
#if !defined(NSPIRE_BUILD)
    submenu_option(&savestate_menu, "Savestate options",
     "Select to enter a menu for loading, saving, and\n"
     "viewing the currently active savestate for this game\n"
     "(or to load a savestate file from another game)", 4),
#endif
	submenu_option(&gamepad_config_menu, "Configure gamepad input",
     "Select to change the in-game behavior of buttons\n"
     "and d-pad.", 6),
#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
    submenu_option(&analog_config_menu, "Configure analog input",
     "Select to change the in-game behavior of the PSP analog nub.", 7),
#endif
    submenu_option(&cheats_misc_menu, "Cheats and Miscellaneous options",
     "Select to manage cheats, set backup behavior,\n"
     "and set device clock speed.", 9),
    action_option(menu_load, NULL, "Load new game",
     "Select to load a new game\n"
     "(will exit a game if currently playing).", 11),
    action_option(menu_restart, NULL, "Restart game",
     "Select to reset the GBA with the current game\n"
     "loaded.", 12),
    action_option(menu_exit, NULL, "Return to game",
     "Select to exit this menu and resume gameplay.", 13),
    action_option(menu_quit, NULL, "Exit gpSP",
     "Select to exit gpSP and return to the menu.", 15)
  };

  make_menu(main, submenu_main, NULL);

  void choose_menu(menu_type *new_menu)
  {
    if(new_menu == NULL)
      new_menu = &main_menu;

    clear_screen(COLOR_BG);

#if !defined(GP2X_BUILD) && !defined(NSPIRE_BUILD)
    blit_to_screen(original_screen, 240, 160, 230, 40);
#endif

    current_menu = new_menu;
    current_option = new_menu->options;
    current_option_num = 0;
    if(current_menu->init_function)
     current_menu->init_function();
  }

  void clear_help()
  {
    for(i = 0; i < 6; i++)
    {
      print_string_pad(" ", COLOR_BG, COLOR_BG, 8, 210 + (i * 10), 70);
    }
  }
  
  menu_update_clock();
  video_resolution_large();

#ifndef NSPIRE_BUILD
#ifndef GP2X_BUILD
  SDL_LockMutex(sound_mutex);
#endif
  SDL_PauseAudio(1);

#ifndef GP2X_BUILD
  SDL_UnlockMutex(sound_mutex);
#endif
#endif

  if(gamepak_filename[0] == 0)
  {
    first_load = 1;
    memset(original_screen, 0x00, 240 * 160 * 2);
    print_string_ext("No game loaded yet.", 0xFFFF, 0x0000,
     60, 75,original_screen, 240, 0, 0, FONT_HEIGHT);
  }

  choose_menu(&main_menu);

  for(i = 0; i < 10; i++)
  {
    if(i >= num_cheats)
    {
      sprintf(cheat_format_str[i], "cheat %d (none loaded)", i);
    }
    else
    {
      sprintf(cheat_format_str[i], "cheat %d (%s): %%s", i,
       cheats[i].cheat_name);
    }
  }

  //current_menu->init_function();

  while(repeat)
  {
    display_option = current_menu->options;
#ifdef NSPIRE_BUILD
	clear_screen(COLOR_BG);
#endif
    current_menu->init_function();
	for(i = 0; i < current_menu->num_options; i++, display_option++)
    {
      if(display_option->option_type & NUMBER_SELECTION_OPTION)
      {
        sprintf(line_buffer, display_option->display_string,
         *(display_option->current_option));
      }
      else

      if(display_option->option_type & (STRING_SELECTION_OPTION))
      {
        sprintf(line_buffer, display_option->display_string,
         ((u32 *)display_option->options)[*(display_option->current_option)]);
      }
      else
      {
        strcpy(line_buffer, display_option->display_string);
      }

      if(display_option == current_option)
      {
        print_string_pad(line_buffer, COLOR_ACTIVE_ITEM, COLOR_BG, 10,
         (display_option->line_number * 10) + 40, 41);
      }
      else
      {
        print_string_pad(line_buffer, COLOR_INACTIVE_ITEM, COLOR_BG, 10,
         (display_option->line_number * 10) + 40, 41);
      }
    }

    print_string(current_option->help_string, COLOR_HELP_TEXT,
     COLOR_BG, 8, 210);

    flip_screen();
#ifdef NSPIRE_BUILD
	update_at_vblank();
#endif

    gui_action = get_gui_input();

    switch(gui_action)
    {
      case CURSOR_DOWN:
        current_option_num = (current_option_num + 1) %
          current_menu->num_options;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_UP:
        if(current_option_num)
          current_option_num--;
        else
          current_option_num = current_menu->num_options - 1;

        current_option = current_menu->options + current_option_num;
        clear_help();
        break;

      case CURSOR_RIGHT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          *(current_option->current_option) =
           (*current_option->current_option + 1) %
           current_option->num_options;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_LEFT:
        if(current_option->option_type & (NUMBER_SELECTION_OPTION |
         STRING_SELECTION_OPTION))
        {
          u32 current_option_val = *(current_option->current_option);

          if(current_option_val)
            current_option_val--;
          else
            current_option_val = current_option->num_options - 1;

          *(current_option->current_option) = current_option_val;

          if(current_option->passive_function)
            current_option->passive_function();
        }
        break;

      case CURSOR_EXIT:
        if(current_menu == &main_menu)
          menu_exit();

        choose_menu(&main_menu);
        break;

      case CURSOR_SELECT:
        if(current_option->option_type & ACTION_OPTION)
          current_option->action_function();

        if(current_option->option_type & SUBMENU_OPTION)
          choose_menu(current_option->sub_menu);
        break;
    }
  }
  
  set_gba_resolution(screen_scale);
  video_resolution_small();
  menu_get_clock_speed();
  set_clock_speed();
#ifndef NSPIRE_BUILD
  SDL_PauseAudio(0);
#endif

  return return_value;
}
