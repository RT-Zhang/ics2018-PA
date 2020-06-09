#include "nemu.h"
#include "device/mmio.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int r = is_mmio(addr);
  if (r == -1)
      return pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3));
  else
      return mmio_read(addr, len, r);
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int r = is_mmio(addr);
  if (r == -1)
      memcpy(guest_to_host(addr), &data, len);
  else
      mmio_write(addr, len, data, r);
}

/*paddr_t page_translate(vaddr_t addr, bool is_write) {
  PDE pde, *pgdir;
  PTE pte, *pgtable;
  paddr_t paddr = addr;
  if (cpu.cr0.protect_enable && cpu.cr0.paging) {
    pgdir = (PDE*)(intprt_t)(cpu.cr3.page_directory_base << 12);
    pde.val = paddr_read((intptr_t)&pgdir[(addr >> 22) & 0x3ff], 4);
    assert(pde.present);
    pde.accessed = 1;

    pgtable = (PTE*)(intptr_t)(pde.page_frame << 12);
    pte.val = paddr_read((intptr_t) & pgtable[(addr >> 12) & 0x3ff], 4);
    assert(pte.present);
    pte.accessed = 1;
    pte.dirty = is_write ? 1 : dirty;
    paddr = (pte.page_frame << 12) | (addr & PAGE_MASK);
  }
  return paddr;
}

bool is_cross_boundry(vaddr_t addr, int len) {
  return (((addr + len - 1) & ~PAGE_MASK) != (addr & ~PAGE_MASK)) ? true : false;
}*/

uint32_t vaddr_read(vaddr_t addr, int len) {
  /*paddr_t paddr;
  if (is_cross_boundry(addr, len)) {
    union {
      uint8_t bytes[4];
      uint32_t dword;
    } data = {0}
    for (int i = 0; i < len; i++) {
      paddr = page_translate(addr + i, false);
      bytes[i] = paddr_read(paddr, 1);
    }
    return data.dword;
  } else {
    paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }*/
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  // paddr_t paddr;
  // if (is_cross_boundry(addr, len)) {
  //   for (int i = 0; i < len; i++) {
  //     paddr = page_translate(addr + i, true);
  //     paddr_write(paddr, 1, data);
  //     data >>= 8;
  //   }
  // } else {
  //   paddr = page_translate(addr, true);
  //   return paddr_write(paddr, len);
  // }
  paddr_write(addr, len, data);
}
