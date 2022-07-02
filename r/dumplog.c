#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <assert.h>
#include <err.h>

#define stat xv6_stat  // avoid clash with host struct stat
#include "types.h"
#include "fs.h"
#include "stat.h"
#include "param.h"

typedef unsigned char byte;

struct logheader {
  int n;
  int block[LOGSIZE];
};

void read_block(FILE *fp, int bn, byte buf[BSIZE])
{
  fseek(fp, BSIZE * bn, SEEK_SET);
  fread(buf, 1, BSIZE, fp);
}

void peek_block(FILE *fp, int bn, int upto)
{
  byte buf[BSIZE];
  read_block(fp, bn, buf);
  for (int i = 0; i < upto; i ++) {
    if (i % 16 == 0)
      printf(" ");
    printf("%c", isprint(buf[i]) ? buf[i] : '.');
  }
}

void load_sb(FILE *fp, struct superblock *sb)
{
  byte buf[BSIZE];
  read_block(fp, 1, buf);

  memcpy(sb, buf, sizeof(*sb));
}

void dump_sb(struct superblock *sb)
{
  printf("> sizeof(superblock) = %zu\n", sizeof(sb));

#define P(F, D) printf("%10s = %08x (%s)\n" , #F, sb->F, D)
  P(size       , "Size of file system image (blocks)");
  P(nblocks    , "Number of data blocks");
  P(ninodes    , "Number of inodes");
  P(nlog       , "Number of log blocks");
  P(logstart   , "Block number of first log block");
  P(inodestart , "Block number of first inode block");
  P(bmapstart  , "Block number of first free map block");
#undef P
}

char *to_file_type(int type)
{
  assert(type < 4);
  char *stype[] = {
    [0] = "N/A",
    [1] = "T_DIR",
    [2] = "T_FILE",
    [3] = "T_DEV",
  };
  return stype[type];
}

void dump_loghead(FILE *fp, struct logheader *lh)
{
  printf("> loghead (n=%d)\n", lh->n);
  for (int i = 0; i < LOGSIZE; i ++) {
    int bn = lh->block[i];
    printf("> %cblock[%02d] = %03d: ",
           (i < lh->n ? '*': ' '), i, bn);
    peek_block(fp, bn, 64);
    printf("\n");
  }
}

void dump_log(FILE *fp, struct superblock *sb)
{
  uint logstart = sb->logstart;
  struct logheader lh;
  fread(&lh, sizeof(lh), 1, fp);
  dump_loghead(fp, &lh);
}

int main(int argc, char *argv[])
{
  FILE *fp = fopen("fs.img", "r");
  if (!fp)
    err(1, "failed to open fs.img");

  struct superblock sb;
  load_sb(fp, &sb);

  dump_sb(&sb);
  dump_log(fp, &sb);

  fclose(fp);
  return 0;
}
