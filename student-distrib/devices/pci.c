#include "../lib.h"
#include "pci.h"

#define OFFSET_MASK_LOW 0x03
#define OFFSET_MASK_HIGH 0xFC

// TODO
uint32_t pci_read_long(uint32_t bus, uint32_t slot, uint32_t func, uint32_t reg) {
  pci_addr_t addr;
  addr.enable = 1;
  addr.reserved = 0;
  addr.bus_number = bus;
  addr.dev_number = slot;
  addr.function = func;
  addr.offset = reg << 2;
  outl(addr.value, PCI_CFG_ADDR_PORT);
  return inl(PCI_CFG_DATA_PORT);
}

// TODO
void pci_write_long(uint32_t bus, uint32_t slot, uint32_t func, uint32_t reg, uint32_t data) {
  pci_addr_t addr;
  addr.enable = 1;
  addr.reserved = 0;
  addr.bus_number = bus;
  addr.dev_number = slot;
  addr.function = func;
  addr.offset = reg << 2;
  outl(addr.value, PCI_CFG_ADDR_PORT);
  outl(data, PCI_CFG_DATA_PORT);
}

// TODO
void pci_update(pci_device_t *device, uint32_t offset) {
  uint32_t reg = offset>>2;
  pci_write_long(device->bus, device->slot, device->func, reg, device->regs[reg]);
}

// TODO
uint16_t pci_read_short(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
  uint32_t n = pci_read_long(bus, slot, func, offset >> 2);
  n >>= 2 - (OFFSET_MASK_LOW & offset);
  return n & 0xFFFF;
}

// TODO
void pci_write_short(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset, uint16_t data) {
  uint32_t n = pci_read_long(bus, slot, func, offset >> 2);
  n >>= 2 - (OFFSET_MASK_LOW & offset);
}

// TODO
uint8_t pci_read_byte(uint32_t bus, uint32_t slot, uint32_t func, uint32_t offset) {
  uint32_t n = pci_read_long(bus, slot, func, offset >> 2);
  n >>= 3 - (OFFSET_MASK_LOW & offset);
  return n & 0xFF;
}

// TODO
int find_device(pci_device_t* dev, uint16_t vendor, uint16_t device) {
  uint32_t bus, slot, func=0;
  uint32_t i;
  uint32_t n = (device << 16) | vendor;
  for (bus = 0; bus < 256; bus++) {
    for (slot = 0; slot < 32; slot++) {
      if (pci_read_long(bus, slot, func, /* device/vendor id */ 0) == n) {
        dev->bus = bus;
        dev->slot = slot;
        dev->func = func;
        for (i = 0; i < 16; i++) {
          dev->regs[i] = pci_read_long(bus, slot, func, i);
        }
        for (i = 0; i < 6; i++) {
          if (dev->bar[i].is_port) {
            dev->bar_addr[i] = dev->bar[i].value & PCI_BAR_PORT;
          } else {
            dev->bar_addr[i] = dev->bar[i].value & PCI_BAR_MEM;
          }
        }
        return 0;
      }
    }
  }
  printf("Bummer\n");
  return -1;
}
