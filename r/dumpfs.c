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

void dump_indirect_blocks(FILE *fp, int bn)
{
  printf(" [IN] = %08x\n", bn);

  uint indirect[NINDIRECT];
  assert(sizeof(indirect) == BSIZE);

  read_block(fp, bn, (byte*)indirect);
  for (int i = 0; i < NINDIRECT; i ++) {
    if (indirect[i]) {
      printf("   [%03d] = %08x -> ", i, indirect[i]);
      peek_block(fp, indirect[i], 48);
      printf("\n");
    }
  }
}

void dump_dirents(FILE *fp, struct dinode *d)
{
  assert(d->type == T_DIR);

  for (int i = 0; i < NDIRECT; i ++) {
    int bn = d->addrs[i];
    if (bn == 0)
      continue;

    byte buf[BSIZE];
    read_block(fp, bn, buf);

    for (int j = 0; j < BSIZE/sizeof(struct dirent); j ++) {
      struct dirent *e = (struct dirent*)(buf + j * sizeof(struct dirent));
      if (e->inum)
        printf("    %02d -> %s\n", e->inum, e->name);
    }
  }
}

void dump_inodes(FILE *fp, struct superblock *sb)
{
  for (int ino = 0; ino < sb->ninodes; ino ++) {
    int bn = sb->inodestart + (ino / IPB);
    byte buf[BSIZE];
    read_block(fp, bn, buf);

    int offset = (ino % IPB) * sizeof(struct dinode);
    struct dinode *d = (struct dinode *)(buf + offset);
    if (d->type == 0)
      continue;

    printf("> inode = %d (offset = 0x%x)\n", ino, bn * BSIZE + offset);
    printf("%10s = %-8s (%s)\n", "type", to_file_type(d->type), "File type");
    if (d->type == T_DEV) {
      printf("%10s = %08x (%s)\n", "major", d->major, "Major device number");
      printf("%10s = %08x (%s)\n", "minor", d->minor, "Minor device number");
    }
    printf("%10s = %08x (%s)\n", "nlink", d->nlink, "Number of links to inode in file system");
    printf("%10s = %08x (%s)\n", "size", d->size, "Size of file (bytes)");

    for (int j = 0; j < NDIRECT; j ++) {
      int d_bn = d->addrs[j];
      // skip empty block
      if (d_bn == 0)
        continue;
      printf(" [%02d] = %08x    -> ", j, d_bn);
      peek_block(fp, d_bn, 48);
      printf("\n");
    }
    if (d->addrs[NDIRECT])
      dump_indirect_blocks(fp, d->addrs[NDIRECT]);
    if (d->type == T_DIR)
      dump_dirents(fp, d);
  }
}

int main(int argc, char *argv[])
{
  FILE *fp = fopen("fs.img", "r");
  if (!fp)
    err(1, "failed to open fs.img");

  struct superblock sb;
  load_sb(fp, &sb);

  dump_sb(&sb);
  dump_inodes(fp, &sb);

  fclose(fp);
  return 0;
}
