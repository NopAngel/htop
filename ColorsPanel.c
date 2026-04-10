/*
htop - ColorsPanel.c (Optimized Version)
*/

#include "config.h" 
#include "ColorsPanel.h"
#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include "CRT.h"
#include "FunctionBar.h"
#include "Object.h"
#include "OptionItem.h"
#include "ProvideCurses.h"

static const char* const ColorsFunctions[] = {"      ", "      ", "      ", "      ", "      ", "      ", "      ", "      ", "      ", "Done  ", NULL};

static const char* const ColorSchemeNames[] = {
   "Default", "Monochromatic", "Black on White", "Light Terminal",
   "MC", "Black Night", "Broken Gray", "Nord", NULL
};

static void ColorsPanel_delete(Object* object) {
   Panel_done((Panel*)object);
   free(object);
}

static HandlerResult ColorsPanel_eventHandler(Panel* super, int ch) {
   ColorsPanel* this = (ColorsPanel*) super;
   HandlerResult result = IGNORED;

   switch (ch) {
      case 0x0a: case 0x0d:
      case KEY_ENTER: case KEY_MOUSE:
      case KEY_RECLICK: case ' ': {
         int newMark = Panel_getSelectedIndex(super);
         int oldMark = this->settings->colorScheme;

         assert(newMark >= 0 && newMark < LAST_COLORSCHEME);

         if (newMark != oldMark) {
            CheckItem_set((CheckItem*)Panel_get(super, oldMark), false);
            CheckItem_set((CheckItem*)Panel_get(super, newMark), true);

            this->settings->colorScheme = newMark;
            this->settings->changed = true;
            this->settings->lastUpdate++;

            CRT_setColors(newMark);
            clear(); 
         }
         
         result = HANDLED | REDRAW;
         break;
      }
   }
   return result;
}

const PanelClass ColorsPanel_class = {
   .super = {
      .extends = Class(Panel),
      .delete = ColorsPanel_delete
   },
   .eventHandler = ColorsPanel_eventHandler
};

ColorsPanel* ColorsPanel_new(Settings* settings) {
   ColorsPanel* this = AllocThis(ColorsPanel);
   Panel* super = &this->super;

   FunctionBar* fuBar = FunctionBar_new(ColorsFunctions, NULL, NULL);
   Panel_init(super, 1, 1, 1, 1, Class(CheckItem), true, fuBar);

   this->settings = settings;

   assert(ARRAYSIZE(ColorSchemeNames) == LAST_COLORSCHEME + 1);

   Panel_setHeader(super, "Colors");
   
   for (int i = 0; ColorSchemeNames[i] != NULL; i++) {
      bool isSelected = (i == (int)CRT_colorScheme);
      Panel_add(super, (Object*) CheckItem_newByVal(ColorSchemeNames[i], isSelected));
   }

   return this;
}