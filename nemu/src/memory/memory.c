#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

// address in page table or page directory entry
#define PTE_ADDR(pte) ((uint32_t)(pte) & ~0xfff)

// resolve virtual address
#define PDX(va)     (((uint32_t)(va) >> 22) & 0x3ff)
#define PTX(va)     (((uint32_t)(va) >> 12) & 0x3ff)
#define OFF(va)     ((uint32_t)(va) & 0xfff)

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

paddr_t page_translate(vaddr_t addr, bool is_write) {
  CR0 cr0 = (CR0)cpu.CR0;
  if (cr0.protect_enable && cr0.paging) {
    CR3 cr3 = (CR3)cpu.CR3;

    // page directory
    PDE* pgdirs = (PDE*)PTE_ADDR(cr3.val);
    PDE pde = (PDE)paddr_read((uint32_t)(pgdirs + PDX(addr)), 4);
    Assert(pde.present, "addr=0x%x", addr);

    // page table
    PTE* pgtable = (PTE*)PTE_ADDR(pde.val);
    PTE pte = (PTE)paddr_read((uint32_t)(pgtable + PTX(addr)), 4);
    Assert(pte.present, "addr=0x%x", addr);

    // set accessed and dirty bit
    pde.accessed = 1;
    pte.accessed = 1;
    if (is_write)
      pte.dirty = 1;
    
    // physical address
    paddr_t paddr = PTE_ADDR(pte.val) | OFF(addr);
    //printf("va=0x%x, pa=0x%x\n", addr, paddr);
    return paddr;
  }
  return addr;
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)) {
    int page1 = 0x1000 - OFF(addr);
    int page2 = len - page1;
    paddr_t paddr1 = page_translate(addr, false);
    paddr_t paddr2 = page_translate(addr + page1, false);

    uint32_t low = paddr_read(paddr1, page1);
    uint32_t high = paddr_read(paddr2, page2);
    uint32_t res = high << (page1 << 3) | low;
    return res;
  } else {
    paddr_t paddr = page_translate(addr, false);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if (PTE_ADDR(addr) != PTE_ADDR(addr + len - 1)) {
    int page1 = 0x1000 - OFF(addr);
    int page2 = len - page1;
    paddr_t paddr1 = page_translate(addr, false);
    paddr_t paddr2 = page_translate(addr + page1, false);

    uint32_t low = data & (~0u >> ((4 - page1) << 3));
    uint32_t high = data >> ((4 - page2) << 3);

    paddr_write(paddr1, page1, low);
    paddr_write(paddr2, page2, high);
    return;
  } else {
    paddr_t paddr = page_translate(addr, true);
    paddr_write(paddr, len, data);
  }
}
