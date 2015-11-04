#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <errno.h>

int file_count = 0;
unsigned int g_file_sig[] = {137, 80, 78, 71, 13, 10, 26, 10};
unsigned int g_eof_sig[] = {73, 69, 78, 68, 174, 66, 96, 130};
long long bytes_written = 0;

#define SIG_BYTE_SZ sizeof(g_file_sig)*8
#define MAX_FILE_SIZE 41943040 //40MB
#define OUTDIR "recovered_pngs"

float to_GB (long long v) {
  return ((float)(v))/1024/1024/1024;
}

void checkerr(FILE* f, char* name) {
  if (f == NULL) {
    fprintf(stderr, "File '%s': %s\n", name, strerror(errno));
    exit(1);
  }
}

long long find_signature(FILE* f, unsigned int* sig, int sig_length) {

  int bufsize = 8192;
  unsigned char buf[bufsize];
  int index = 0;
  long long c = 0;
  long long r = 0;

  int looking_for_eof_sig = -1;

  if (memcmp(sig, g_eof_sig, SIG_BYTE_SZ) == 0)
    looking_for_eof_sig = 1;

  while((r = fread(&buf,1,bufsize,f)) == bufsize) { 

    for (int i=0;i<50;i++)
      printf("\b");
    
    printf("read: (%.3fG) writ: (%.3fG)", to_GB(ftell(f)), to_GB(bytes_written));
    fflush(stdout);

    c += r;

    if (looking_for_eof_sig == 1 && c >= MAX_FILE_SIZE ) {
      printf("\nBreaking from corrupted png file\n");
      return -1;
    }

    for (int i=0; i<r;i++) { //look inside the buffer

      unsigned int bint = (unsigned int) buf[i];

      if (index == sig_length-1) {
        long long begin_of_sig = (ftell(f)-r+i+1);
        printf("\nFound sig at 0x%llx (offset %lld)\n", begin_of_sig, begin_of_sig);
        return begin_of_sig;
      }

      //Found something that looks like the sig
      if (sig[index] == bint) {
        index++;
        continue;
      }
      index = 0;
    }
  }

  return -1;
}

void get_file_slice(long long start, long long end, FILE *f) {

  file_count += 1;
  char filename[80];
  snprintf(filename, 30, "%s/%d.png", OUTDIR,file_count);
  printf("Filename is %s\n", filename);

  while(access(filename, F_OK) != -1) {
    printf("WARNING: file %s exists\n", filename);
    strcat(filename, "_");
  }
  
  FILE *outfile = fopen(filename, "w");
  checkerr(outfile, filename);

  printf("File size : %lld\n", end-start);
  fflush(stdout);
  fseeko(f, start, SEEK_SET);
  
  long long bleft = end-start;
  bytes_written += bleft;
  int bufsize = 8192;
  unsigned char buf[bufsize];

  while (bleft > 0) {

    if (bleft < bufsize)
      bufsize = bleft;
    
    long long read = fread(buf, 1, bufsize, f);
    long long writ = fwrite(buf, 1, bufsize, outfile);

    bleft -= writ;
  }

  fseeko(f, end, SEEK_SET);

  fclose(outfile);
}


int main(int argc, char* argv[]) {

  if (argc <= 1) {
    printf("Need file argument\n");
    printf("To undelete pngs on block device 'sda', run the following: \n");
    printf("%s /dev/sda\n", argv[0]);
    exit(1);
  }

  FILE *f = fopen(argv[1], "r");
  checkerr(f, argv[1]);

  int mr = mkdir(OUTDIR, (S_IWUSR | S_IRUSR | S_IXUSR));

  if (mr != 0) {
    fprintf(stderr, "Directory '%s': %s\n", OUTDIR, strerror(errno));
    exit(1);
  }

  while (1) {

    long long file_start = 0;
    long long file_end = 0;
    
    file_start = find_signature(f, g_file_sig, 8)-8;

    if (file_start < 0)
      break;

    printf("png file starts at %lld\n", file_start);

    file_end = find_signature(f, g_eof_sig, 8);

    if (file_end <= 0)
      continue;
    
    printf("End of png is at %lld\n", file_end);

    get_file_slice(file_start, file_end, f);

    if (feof(f) != 0)  {
      printf("END OF FILE!\n");
      break;
    }
  }

  printf("\nExiting\n");

  fclose(f);

  return 0;
}
