/*
htop - Machine.c (Optimized Version)
*/

#include "config.h"
#include "Machine.h"
#include <stdlib.h>
#include <unistd.h>
#include "Object.h"
#include "Platform.h"
#include "Row.h"
#include "XUtils.h"

void Machine_init(Machine* this, UsersTable* usersTable, uid_t userId) {
   this->usersTable = usersTable;
   this->userId = userId;
   this->htopUserId = getuid();
   
   Row_setPidColumnWidth(Platform_getMaxPid());

   // init: Timestamp 
   Platform_gettime_realtime(&this->realtime, &this->realtimeMs);

#ifdef HAVE_LIBHWLOC
   this->topologyOk = false;
   if (hwloc_topology_init(&this->topology) == 0) {
      #if HWLOC_API_VERSION < 0x00020000
         hwloc_topology_ignore_type_keep_structure(this->topology, HWLOC_OBJ_MACHINE);
         hwloc_topology_ignore_type_keep_structure(this->topology, HWLOC_OBJ_CORE);
         hwloc_topology_ignore_type_keep_structure(this->topology, HWLOC_OBJ_CACHE);
         hwloc_topology_set_flags(this->topology, HWLOC_TOPOLOGY_FLAG_WHOLE_SYSTEM);
      #else
         hwloc_topology_set_all_types_filter(this->topology, HWLOC_TYPE_FILTER_KEEP_STRUCTURE);
      #endif
      this->topologyOk = (hwloc_topology_load(this->topology) == 0);
   }
#endif
}

void Machine_done(Machine* this) {
#ifdef HAVE_LIBHWLOC
   if (this->topologyOk) {
      hwloc_topology_destroy(this->topology);
   }
#endif
   Object_delete(this->processTable);
   free(this->tables);
}

static void Machine_addTable(Machine* this, Table* table) {
   for (size_t i = 0; i < this->tableCount; i++) {
      if (this->tables[i] == table) return;
   }

   this->tables = xReallocArray(this->tables, this->tableCount + 1, sizeof(Table*));
   this->tables[this->tableCount++] = table;
}

void Machine_populateTablesFromSettings(Machine* this, Settings* settings, Table* processTable) {
   this->settings = settings;
   this->processTable = processTable;

   if (settings->nScreens > 0 && !this->tables) {
      this->tables = xCalloc(settings->nScreens, sizeof(Table*));
   }

   for (size_t i = 0; i < settings->nScreens; i++) {
      ScreenSettings* ss = settings->screens[i];

      if (!ss->table)
         ss->table = processTable;

      if (i == 0)
         this->activeTable = ss->table;

      Machine_addTable(this, ss->table);
   }
}

void Machine_setTablesPanel(Machine* this, Panel* panel) {
   for (size_t i = 0; i < this->tableCount; i++) {
      Table_setPanel(this->tables[i], panel);
   }
}

void Machine_scanTables(Machine* this) {
   static bool firstScanDone = false;

   if (firstScanDone) {
      this->prevMonotonicMs = this->monotonicMs;
      Platform_gettime_monotonic(&this->monotonicMs);
   } else {
      this->prevMonotonicMs = 0;
      this->monotonicMs = 1;
      firstScanDone = true;
   }

   if (this->monotonicMs <= this->prevMonotonicMs && firstScanDone) {
      return;
   }

   this->maxUserId = 0;
   Row_resetFieldWidths();

   for (size_t i = 0; i < this->tableCount; i++) {
      Table* table = this->tables[i];

      Table_scanPrepare(table);
      Table_scanIterate(table);
      Table_scanCleanup(table);
   }

   Row_setUidColumnWidth(this->maxUserId);
   Row_setPidColumnWidth(this->maxProcessId);
}