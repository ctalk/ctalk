/* $Id: shmem.c,v 1.3 2019/11/11 20:22:54 rkiesling Exp $ -*-c-*-*/

/*
  This file is part of Ctalk.
  Copyright © 2005-2012, 2015  Robert Kiesling, rk3314042@gmail.com.
  Permission is granted to copy this software provided that this copyright
  notice is included in all source code modules.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software Foundation, 
  Inc., 51 Franklin St., Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/signal.h>
#include <unistd.h>
#include "object.h"
#include "message.h"
#include "ctalk.h"
#include "rtinfo.h"

#ifndef DJGPP
#include <sys/shm.h>
#include "list.h"

#define SHM_BLKSIZE 10240  /* If changing, also change in x11lib.c, etc... */

/* #define DEBUG 1 */

LIST *shm_handles = NULL;

typedef struct {
  int handle;
  char path[FILENAME_MAX];
} SHM_DATA;

SHM_DATA *new_shm_data (void) {
  SHM_DATA *p;
  if ((p = (SHM_DATA *)__xalloc (sizeof (SHM_DATA))) == NULL) {
    fprintf (stderr, "new_shm_data: %s.\n", strerror (errno));
    return NULL;
  }
  return p;
}

static char *shm_handle_path (void) {
  static char s[FILENAME_MAX];
  int pid;
  FILE *f;
  pid = (int)getpid ();
  sprintf (s, "%s/.%d", P_tmpdir, pid);
  while (file_exists (s)) {
    ++pid;
    sprintf (s, "%s/.%d", P_tmpdir, pid);
  }

  if ((f = fopen (s, "w")) == NULL) {
    _warning ("shm_handle_path (fopen): %s.\n", strerror (errno));
    kill (0, SIGTERM);
  }
  fwrite ("", 0, 0, f);
  fclose (f);
  return s;
}

int mem_setup (void) {
  key_t key;
  int id;
  char path[FILENAME_MAX];
  SHM_DATA *s;
  LIST *l;

  strcpy (path, shm_handle_path ());
  if ((key = ftok (path, 'a')) == -1) {
    _warning ("mem_setup (ftok): %s.\n", strerror (errno));
    exit (ERROR);
  }
  if ((id = shmget (key, SHM_BLKSIZE, 0644 | IPC_CREAT)) == -1) {
    _warning ("mem_setup (shmget): %s.\n", strerror (errno));
    exit (ERROR);
  }
  s = new_shm_data ();
  s -> handle = id;
  strcpy (s -> path, path);

  l = new_list ();
  l -> data = s;
  list_push (&shm_handles, &l);

  return id;
}

#if defined(__sparc__) && defined(__svr4__)
#define SHMAT_FLAGS SHM_SHARE_MMU
#else /* Linux */
#define SHMAT_FLAGS 0
#endif

void *get_shmem (int handle) {
  void *v;
  if (handle == 0)
    return NULL;
  if ((v = shmat (handle, (void *)0, SHMAT_FLAGS)) == (void *)(-1)) {
    /* an error here is basically unrecoverable. */
    fprintf (stderr, "Shared memory error: get_shmem.\n");
    kill (-1, SIGKILL);
  }
  return v;
}

int detach_shmem (int mem_id, void * shmem) {
  struct shmid_ds s;
  shmctl (mem_id, IPC_STAT, &s);
  if (shmdt (shmem) < 0) {
    /* here, too - a basically unrecoverable error. */
    fprintf (stderr, "Shared memory error: detach_shmem.\n");
    kill (-1, SIGKILL);
  } else {
    return s.shm_nattch;
  }
  return -1;
}

void release_shmem (int mem_id, void * shmem) {
  struct shmid_ds s;
  
  shmctl (mem_id, IPC_STAT, &s);
#if DEBUG
  fprintf (stderr, "release_shmem: memory attached to %d processes.\n",
	   (int)s.shm_nattch);
#endif
  while (s.shm_nattch > 0) {
    if (shmdt (shmem) < 0) {
#if DEBUG
      fprintf (stderr, "release_shmem: %s.\n", strerror (errno));
#endif
      return;
    } else {
      shmctl (mem_id, IPC_STAT, &s);
#if DEBUG
      fprintf (stderr, "release_shmem: memory attached to %d processes.\n",
	       (int)s.shm_nattch);
#endif
    }
  }
  shmctl (mem_id, IPC_RMID, &s);
  delete_shmem_info (mem_id);
}

void delete_shmem_info (int handle) {
  LIST *l;
  SHM_DATA *s;
  for (l = shm_handles; l ; l = l -> next) {
    s = (SHM_DATA*)l -> data;
    if (s -> handle == handle) {
      unlink (s -> path);
      list_remove (&shm_handles, &l);
      __xfree (MEMADDR(l));
      __xfree (MEMADDR(s));
      return;
    }
  }
}

#else /* #ifndef DJGPP */
#warning Shared memory functions are not implemented for Win32.
#endif /* #ifndef DJGPP */
