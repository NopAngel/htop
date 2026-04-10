/*
htop - LineEditor.c (Optimized Version)
*/

#include "config.h"
#include "LineEditor.h"
#include <ctype.h>
#include <string.h>
#include "CRT.h"
#include "Panel.h"
#include "ProvideCurses.h"

#define BUFFER_REMAINING(this) ((this)->len - (this)->cursor)

void LineEditor_init(LineEditor* this) {
   LineEditor_initWithMax(this, LINEEDITOR_MAX);
}

void LineEditor_initWithMax(LineEditor* this, size_t maxLen) {
   memset(this->buffer, 0, sizeof(this->buffer)); // cleaned
   this->len = 0;
   this->cursor = 0;
   this->scroll = 0;
   this->maxLen = (maxLen > 0 && maxLen < LINEEDITOR_MAX) ? maxLen : (LINEEDITOR_MAX - 1);
}

void LineEditor_reset(LineEditor* this) {
   this->buffer[0] = '\0';
   this->len = 0;
   this->cursor = 0;
   this->scroll = 0;
}

void LineEditor_setText(LineEditor* this, const char* text) {
   if (!text) text = "";
   
   size_t n = strlen(text);
   if (n > this->maxLen) n = this->maxLen;
   
   memcpy(this->buffer, text, n);
   this->buffer[n] = '\0';
   this->len = n;
   this->cursor = n;
   this->scroll = 0;
}

static void moveCursorWordLeft(LineEditor* this) {
   size_t pos = this->cursor;
   const unsigned char* buf = (const unsigned char*)this->buffer;
   
   while (pos > 0 && isspace(buf[pos - 1])) pos--;
   while (pos > 0 && !isspace(buf[pos - 1])) pos--;
   
   this->cursor = pos;
}

static void moveCursorWordRight(LineEditor* this) {
   size_t pos = this->cursor;
   size_t limit = this->len;
   const unsigned char* buf = (const unsigned char*)this->buffer;
   
   while (pos < limit && !isspace(buf[pos])) pos++;
   while (pos < limit && isspace(buf[pos])) pos++;
   
   this->cursor = pos;
}

static bool deleteCharBefore(LineEditor* this) {
   if (this->cursor == 0) return false;
   
   this->cursor--;
   memmove(this->buffer + this->cursor, this->buffer + this->cursor + 1, BUFFER_REMAINING(this) + 1);
   this->len--;
   return true;
}

static bool deleteCharAt(LineEditor* this) {
   if (this->cursor >= this->len) return false;
   
   memmove(this->buffer + this->cursor, this->buffer + this->cursor + 1, BUFFER_REMAINING(this));
   this->len--;
   return true;
}

static bool insertChar(LineEditor* this, char ch) {
   if (this->len >= this->maxLen) return false;

   memmove(this->buffer + this->cursor + 1, this->buffer + this->cursor, BUFFER_REMAINING(this) + 1);
   this->buffer[this->cursor] = ch;
   this->cursor++;
   this->len++;
   return true;
}

bool LineEditor_handleKey(LineEditor* this, int ch) {
   switch (ch) {
      case KEY_LEFT:  case KEY_CTRL('B'): if (this->cursor > 0) this->cursor--; return false;
      case KEY_RIGHT: case KEY_CTRL('F'): if (this->cursor < this->len) this->cursor++; return false;
      case KEY_HOME:  case KEY_CTRL('A'): this->cursor = 0; return false;
      case KEY_END:   case KEY_CTRL('E'): this->cursor = this->len; return false;
      case KEY_SLEFT:  moveCursorWordLeft(this); return false;
      case KEY_SRIGHT: moveCursorWordRight(this); return false;
      case KEY_DC:     return deleteCharAt(this);
      case KEY_BACKSPACE: case 127: return deleteCharBefore(this);

      case KEY_CTRL('W'): {
         size_t end = this->cursor;
         moveCursorWordLeft(this); 
         if (this->cursor == end) return false;

         size_t deleted = end - this->cursor;
         memmove(this->buffer + this->cursor, this->buffer + end, this->len - end + 1);
         this->len -= deleted;
         return true;
      }

      case KEY_CTRL('K'):
         if (this->cursor >= this->len) return false;
         this->buffer[this->cursor] = '\0';
         this->len = this->cursor;
         return true;

      case KEY_CTRL('U'):
         if (this->cursor == 0) return false;
         memmove(this->buffer, this->buffer + this->cursor, this->len - this->cursor + 1);
         this->len -= this->cursor;
         this->cursor = 0;
         return true;

      default:
         if (ch > 0 && ch < 256 && isprint((unsigned char)ch)) {
            return insertChar(this, (char)ch);
         }
         return false;
   }
}

int LineEditor_draw(LineEditor* this, int startX, int fieldWidth, int attr) {
   attrset(attr == -1 ? CRT_colors[FUNCTION_BAR] : attr);

   int visibleLen = (int)this->len - (int)this->scroll;
   if (visibleLen < 0) visibleLen = 0;
   if (visibleLen > fieldWidth) visibleLen = fieldWidth;

   // draw a text
   mvaddnstr(LINES - 1, startX, this->buffer + this->scroll, visibleLen);

   if (visibleLen < fieldWidth) {
      printw("%*s", fieldWidth - visibleLen, "");
   }

   return startX + (int)(this->cursor - this->scroll);
}

void LineEditor_updateScroll(LineEditor* this, int fieldWidth) {
   if (fieldWidth <= 0)
      return;
   size_t fw = (size_t)fieldWidth;
   if (this->cursor < this->scroll) {
      this->scroll = this->cursor;
   } else if (this->cursor >= this->scroll + fw) {
      this->scroll = this->cursor - fw + 1;
   }
}

void LineEditor_click(LineEditor* this, int clickX, int fieldStartX) {
   int offset = clickX - fieldStartX;
   if (offset < 0)
      offset = 0;
   size_t newCursor = this->scroll + (size_t)offset;
   if (newCursor > this->len)
      newCursor = this->len;
   this->cursor = newCursor;
}