/*
    splitelf: Extract trailing data from an ELF file.
    Copyright (C) 2017  Valentin David

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <elf.h>

int main(int argc, char *argv[]) {
  if (argc < 3) {
    fprintf(stderr, "slitpelf: missing arguments");
    return 1;
  }
  int infd = open(argv[1], O_RDONLY);
  if (infd < 0) {
    perror("splitelf");
    return 1;
  }
  int outfd = open(argv[2], O_WRONLY|O_CREAT, 0666);
  if (outfd < 0) {
    perror("splitelf");
    return 1;
  }
  Elf64_Ehdr header;
  ssize_t r = read(infd, (void*)&header, sizeof(Elf64_Ehdr));
  if (r < 0) {
    perror("splitelf");
    return 1;
  } else if (r != sizeof(Elf64_Ehdr)) {
    fprintf(stderr, "slitpelf: not enough bytes in elf header");
    return 1;
  }
  off_t elf_size = (off_t)(header.e_shoff + header.e_shentsize*header.e_shnum);
  off_t seeked = lseek(infd, (off_t)elf_size, SEEK_SET);
  if (seeked < 0) {
    perror("splitelf");
    return 1;
  } else if (seeked != elf_size) {
    fprintf(stderr, "slitpelf: not enough bytes in elf");
    return 1;
  }
  while (1) {
    char data[4096];
    ssize_t dr = read(infd, &data, 4096);
    if (dr < 0) {
      perror("splitelf");
      return 1;
    }
    write(outfd, &data, dr);
    if (dr != 4096) {
      break ;
    }
  }
  close(infd);
  close(outfd);
  return 0;
}
