#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>
#include <sys/time.h>

#define MAX_MIXOPS	100

struct entry {
  int key;
  int value;
  struct entry *next;
};

struct entry **table;
int *keys;
int nbucket = 5;
int nkey = 100000;
int nthread = 1;
int seed = 0;

volatile int bput = 0;
volatile int bget = 0;
volatile int bmix = 0;

double
now(void)
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  return tv.tv_sec + tv.tv_usec / 1000000.0;
}

static
void put(int key, int value)
{
  int b = key % nbucket;
  struct entry *e = malloc(sizeof(struct entry));

  e->key = key;
  e->value = value;
  e->next = table[b];
  table[b] = e;
}

static struct entry*
get(int key)
{
  struct entry *e = 0;
  for (e = table[key % nbucket]; e != 0; e = e->next) {
    if (e->key == key) break;
  }
  return e;
}

void
barrier(volatile int *done)
{
  __sync_fetch_and_add(done, 1);
  while (*done < nthread) ;
}

static void *
thread(void *xa)
{
  long n = (long) xa;
  int b = nkey/nthread;
  double t1, t0;

  t0 = now();
  for (int i = 0; i < b; i++) {
    put(keys[b*n + i], n);
  }
  t1 = now();
  printf("%ld: put time = %f\n", n, t1-t0);

  barrier(&bput);

  t0 = now();
  int k = 0;
  for (int i = 0; i < nkey; i++) {
    struct entry *e = get(keys[i]);
    if (e == 0)
			k++;
  }
  t1 = now();

  printf("%ld: lookup time = %f\n", n, t1-t0);
  printf("%ld: %d keys missing\n", n, k);

  barrier(&bget);

	t0 = now();
	k = 0;
	for (int i = 0; i < MAX_MIXOPS; i++) {
		put(keys[random() % nkey], i);
		struct entry *e = get(keys[i % nkey]);
		if (e == 0)
			k++;
	}
	t1 = now();

	printf("%ld: mix of put and lookup time = %f\n", n, t1-t0);
	printf("%ld: %d keys missing\n", n, k);

	barrier(&bmix);

  return NULL;
}

int
main(int argc, char *argv[])
{
  pthread_t *tids;
  double t1, t0;

  char c;
  while ((c = getopt (argc, argv, "c:s:b:k:")) != -1) {
    switch (c) {
    case 'c':
      nthread = atoi(optarg);
      break;
    case 's':
      seed = atoi(optarg);
      break;
    case 'b':
      nbucket = atoi(optarg);
      break;
    case 'k':
      nkey = atoi(optarg);
      break;
    default:
      printf("[usage] %s -c [cpu] -s [seed] -b [buckets] -k [keys]\n", argv[0]);
      return 1;
    }
  }

  table = calloc(nbucket, sizeof(struct entry *));
  assert(table);
  tids = calloc(nthread, sizeof(pthread_t));
  assert(tids);
  keys = calloc(nkey, sizeof(int));
  assert(keys);

  srandom(seed);
  assert(nkey % nthread == 0);

  for (int i = 0; i < nkey; i++)
    keys[i] = random();

  t0 = now();
  for (int i = 0; i < nthread; i++)
    assert(pthread_create(&tids[i], NULL, thread, (void *)(long)(i)) == 0);
  for (int i = 0; i < nthread; i++)
    assert(pthread_join(tids[i], NULL) == 0);
  t1 = now();

  printf("completion time = %f\n", t1-t0);
}
