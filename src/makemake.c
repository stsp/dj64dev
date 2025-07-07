/* Copyright (C) 2015 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1998 DJ Delorie, see COPYING.DJ for details */
/* Copyright (C) 1995 DJ Delorie, see COPYING.DJ for details */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

static FILE *mf, *oi, *rf, *rfo;
static char _starting_dir[2000];
static char _top_dir[2000];
static char path[2000];
static int do_oh_files = 0;

void
process_makefile(char *path_end, const char *mk, FILE *rf)
{
  FILE *oh;
  int last_was_nl = 1;
  int ch;

  strcpy(path_end, mk);
  oh = fopen(path, "r");
  if (oh == 0)
    return;

  *path_end = 0;

  while ((ch = fgetc(oh)) != EOF)
  {
    if (ch == '\n' && last_was_nl)
      continue;
    if (ch != '\n' && last_was_nl)
      fprintf(oi, "OBJS += ");
    last_was_nl = (ch == '\n');
    if (ch == '&')
    {
      fprintf(oi, "%s", path+2);
      fprintf(rf, "%s", path+2);
    }
    else
    {
      fputc(ch, oi);
      fputc(ch, rf);
    }
  }
  fclose(oh);
}

int
sort_alpha(const void *va, const void *vb)
{
  char *a = *(char **)va;
  char *b = *(char **)vb;
  return strcmp(a, b);
}

void
find(void)
{
  char *path_end;
  DIR *d;
  struct dirent *de;
  char **subs;
  int nsubs=0, msubs=10, i;
  struct stat s;

  path_end = path+strlen(path);

  /* Find all subdirectories and any makefile */
  d = opendir(path);
  if (d == 0)
  {
    fprintf(stderr, "Error: cannot open %s\n", path);
    exit(1);
  }
  *path_end = '/';
  subs = (char **)malloc(msubs*sizeof(char *));
  while ((de = readdir(d)) != NULL) {

    /* Skip . and .. */
    if (de->d_name[0] == '.')
      continue;

    /* optimization: no djgpp src directories have dots,
       so skip stat() for them */
    if (strchr(de->d_name, '.'))
      continue;

    /* See if what we found is a directory */
    strcpy(path_end+1, de->d_name);
    stat(path, &s);
    if (S_ISDIR(s.st_mode))
    {
      if (nsubs >= msubs)
      {
	msubs += 10;
	subs = (char **)realloc(subs, msubs * sizeof(char *));
      }
      subs[nsubs] = (char *)malloc(strlen(de->d_name)+1);
      strcpy(subs[nsubs], de->d_name);
      nsubs++;
    }

    /* Don't check the top directory! */
    if (strcmp(path, "./makefile") == 0)
      continue;

    /* See if the current directory has a makefile */
    if (strcmp(de->d_name, "makefile") == 0)
    {
      if (!do_oh_files)
      {
        *path_end = 0;
        if (path[1])
        {
          /*      fprintf(mf, "\t@echo =---------- make $(SUBARGS) in %s\n", path+2); */
          fprintf(mf, "\t$(MAKE) -C %s $(SUBARGS)\n", path+2);
        }
      }
      else
      {
        process_makefile(path_end, "/makefile.oh", rf);
        process_makefile(path_end, "/makefile.oho", rfo);
      }
      *path_end = '/';
    }
  }
  closedir(d);


  /* Now, descend into subdirectories, in asciibetical order */
  qsort(subs, nsubs, sizeof(char *), sort_alpha);
  for (i=0; i<nsubs; i++)
  {
    strcpy(path_end+1, subs[i]);
    find();
  }
  *path_end = 0;
}

void
move_if_change(char *src, char *dest)
{
  FILE *s, *d;
  s = fopen(src, "r");
  d = fopen(dest, "r");
  if (!d)
  {
    fclose(s);
    rename(src, dest);
    return;
  }
  while (1) {
    int sc = fgetc(s);
    int dc = fgetc(d);
    if (sc != dc) {
      fclose(s);
      fclose(d);
      rename(src, dest);
      return;
    }
    if (sc == EOF)
      break;
  }
  remove(src);
}

int
main(int argc, char **argv)
{
  char *notepwd;
  char *starting_dir, *top_dir;
  int err;

  starting_dir = getcwd(_starting_dir, 200);
  if (!starting_dir)
    return EXIT_FAILURE;

  if (argc > 2 && strcmp(argv[1], "-C") == 0)
  {
    err = chdir(argv[2]);
    if (err)
      return EXIT_FAILURE;
    argc -= 2;
    argv += 2;
  }

  top_dir = getcwd(_top_dir, 200);
  if (!top_dir)
    return EXIT_FAILURE;
  notepwd = strrchr(top_dir, '/');
  if (notepwd)
    notepwd++;
  else
    notepwd = top_dir;

  if (argc > 1 && strcmp(argv[1], "-2") == 0)
  {
    do_oh_files = 1;
    printf("makemake: scanning %s for object files\n", notepwd);
  }
  else
    printf("makemake: scanning %s for makefiles\n", notepwd);

  if (!do_oh_files)
    mf = fopen("makefile.sub", "w");
  else
  {
    oi = fopen("makefile.oi", "w");
    rf = fopen("makefile.rf2", "w");
    rfo = fopen("makefile.rf3", "w");
  }

  if (!do_oh_files)
    fprintf(mf, "all_subs:\n");

  strcpy(path, ".");
  find();

  err = chdir(starting_dir);
  if (err)
    return EXIT_FAILURE;

  if (!do_oh_files)
    fclose(mf);
  else
  {
    fclose(oi);
    fclose(rf);
    fclose(rfo);
    move_if_change("makefile.rf2", "makefile.rf");
    move_if_change("makefile.rf3", "makefile.rfo");
  }

  return 0;
}
