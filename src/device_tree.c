//
// Created by Bryan Keller on 3/31/25.
//

#include "boot_args.h"
#include "bootmii_ppc.h"
#include "device_tree.h"
#include "macho_decoder.h"
#include "string.h"
#include "fs.h"
#include "driver_loader.h"

struct DTProperty {
  char name[32];                        // NUL terminated property name
  u32 length;                           // Length (bytes) of property value that follows
  // unsigned long value[1];            // Variable length value of property
  // Padded to a multiple of a longword
};
typedef struct DTProperty DTProperty, *DTPropertyPtr;

struct DTNode {
  u32 nProperties;                      // Number of props[] elements (0 => end of list)
  u32 nChildren;                        // Number of children[] elements
  // DTProperty props[];                // nProperties DTProperty structs
  // DTNode children[];                 // nChildren DTNode structs
};
typedef struct DTNode DTNode, *DTNodePtr;

static DTNode* create_node(u32 nProps, u32 nChildren) {
  DTNode* node = (DTNode*)device_tree_end;
  node->nProperties = nProps;
  node->nChildren = nChildren;
  
  // Advance pointer past the node header
  device_tree_end += sizeof(DTNode);
  return node;
}

// Add one property to the current node
static void add_property(const char* name, const void* data, u32 length) {
  DTProperty* prop = (DTProperty*)device_tree_end;
  strlcpy(prop->name, name, sizeof(prop->name));
  prop->length = length;
  
  // Advance pointer past the property header
  device_tree_end += sizeof(DTProperty);
  
  // Copy the property data bytes
  memcpy((void*)device_tree_end, data, length);
  
  // Round up to next multiple of 4 (if needed)
  u32 paddedLength = (length + 3) & ~3;
  
  // Advance pointer by property data length
  device_tree_end += paddedLength;
}

void build_device_tree() {
  device_tree_start = boot_args_address + sizeof(boot_args_t);
  device_tree_end = device_tree_start;
  
  // /
  create_node(/*nProps=*/5, /*nChildren=*/6);
  {
    const char *name = "device-tree";
    add_property("name", name, strlen(name) + 1);
    
    const char *model = "iMac,1";
    add_property("model", model, strlen(model) + 1);
    
    const char compatible_root[] =
    "iMac,1\0"
    "MacRISC\0"
    "PowerMac\0";
    
    add_property("compatible", compatible_root, sizeof(compatible_root));
    
    u32 address_cells = 1;
    add_property("#address-cells", &address_cells, sizeof(address_cells));
    
    u32 size_cells = 1;
    add_property("#size-cells", &size_cells, sizeof(size_cells));
    
    // /cpus
    create_node(/*nProps=*/3, /*nChildren=*/1);
    {
      const char *name = "cpus";
      add_property("name", name, strlen(name) + 1);
      
      u32 address_cells = 1;
      add_property("#address-cells", &address_cells, sizeof(address_cells));
      
      u32 size_cells = 0;
      add_property("#size-cells", &size_cells, sizeof(size_cells));
      
      // /cpus/PowerPC,G3
      create_node(/*nProps=*/14, /*nChildren=*/0);
      {
        const char *name = "PowerPC,G3";
        add_property("name", name, strlen(name) + 1);
        
        u32 reg[1] = { 0x00000000 };
        add_property("reg", reg, sizeof(reg));
        
        const char *device_type = "cpu";
        add_property("device_type", device_type, strlen(device_type) + 1);
        
        u32 clock_frequency = 729000000; // Wii's Broadway CPU ~729MHz
        add_property("clock-frequency", &clock_frequency, sizeof(clock_frequency));
        
        u32 tb_frequency = 60750000; // 243MHz / 4
        add_property("timebase-frequency", &tb_frequency, sizeof(tb_frequency));
        
        u32 bus_frequency = 243000000; // 729000000 / 3
        add_property("bus-frequency", &bus_frequency, sizeof(bus_frequency));
        
        u32 cpu_version = 0x00087102;
        add_property("cpu-version", &cpu_version, sizeof(cpu_version));
        
        u32 d_cache_size = 0x00008000;
        add_property("d-cache-size", &d_cache_size, sizeof(d_cache_size));
        
        u32 i_cache_size = 0x00008000;
        add_property("i-cache-size", &i_cache_size, sizeof(i_cache_size));
        
        u32 d_cache_block_size = 0x00000020;
        add_property("d-cache-block-size", &d_cache_block_size, sizeof(d_cache_block_size));
        
        u32 i_cache_block_size = 0x00000020;
        add_property("i-cache-block-size", &i_cache_block_size, sizeof(i_cache_block_size));
        
        u32 d_cache_sets = 0x00000080;
        add_property("d-cache-sets", &d_cache_sets, sizeof(d_cache_sets));
        
        u32 i_cache_sets = 0x00000080;
        add_property("i-cache-sets", &i_cache_sets, sizeof(i_cache_sets));
        
        u32 reservation_granularity = 0x00000020;
        add_property("reservation-granularity", &reservation_granularity, sizeof(reservation_granularity));
      }
    }
    
    // /memory
    create_node(/*nProps=*/3, /*nChildren=*/0);
    {
      const char *name = "memory";
      add_property("name", name, strlen(name) + 1);
      
      const char *device_type = "memory";
      add_property("device_type", device_type, strlen(device_type) + 1);
      
      // I don't think this is used or looked at, and might just be used in an OF Fourth environment
      u32 reg[4] = {
        0x00000000, 0x01800000, // 24 MB MEM1
        0x10000000, 0x04000000  // 64 MB MEM2
      };
      add_property("reg", reg, sizeof(reg));
    }
    
    // /hollywood
    create_node(/*nProps=*/6, /*nChildren=*/7);
    {
      const char* name = "hollywood";
      add_property("name", name, strlen(name) + 1);
      
      const char *device_type = "hollywood";
      add_property("device_type", device_type, strlen(device_type) + 1);
      
      const char *compatible = "nintendo,hollywood";
      add_property("compatible", compatible, strlen(compatible) + 1);
      
      u32 address_cells = 1;
      add_property("#address-cells", &address_cells, sizeof(address_cells));
      
      u32 size_cells = 1;
      add_property("#size-cells", &size_cells, sizeof(size_cells));
      
      u32 ranges[9] = {
        0x0c000000, 0x0c000000, 0x01000000,
        0x0d000000, 0x0d000000, 0x00800000,
        0x0d800000, 0x0d800000, 0x00800000,
      };
      add_property("ranges", ranges, sizeof(ranges));
      
      // /hollywood/video@0c002000
      create_node(/*nProps=*/5, /*nChildren=*/0);
      {
        const char *name = "video";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-vi";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0c002000, 0x100
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupts = 8;
        add_property("interrupts", &interrupts, sizeof(interrupts));
        
        u32 interrupt_parent = 0xFEAD0000;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
      }
      
      // /hollywood/pic0@0c003000
      create_node(/*nProps=*/6, /*nChildren=*/0);
      {
        const char *name = "pic0";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,flipper-pic";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0c003000, 0x00000100
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupt_cells = 1;
        add_property("#interrupt-cells", &interrupt_cells, sizeof(interrupt_cells));
        
        u32 phandle = 0xFEAD0000;
        add_property("AAPL,phandle", &phandle, sizeof(phandle));
        
        add_property("interrupt-controller", NULL, 0);
      }
      
      // /hollywood/exi@0d006800
      create_node(/*nProps=*/5, /*nChildren=*/0);
      {
        const char *name = "exi";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-exi";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0d006800, 0x00000040
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupts = 4;
        add_property("interrupts", &interrupts, sizeof(interrupts));
        
        u32 interrupt_parent = 0xFEAD0000;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
      }
      
      // /hollywood/ehci@0d040000
      create_node(/*nProps=*/5, /*nChildren=*/0);
      {
        const char *name = "ehci";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-usb-ehci";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0d040000, 0x00000100
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupts = 4;
        add_property("interrupts", &interrupts, sizeof(interrupts));
        
        u32 interrupt_parent = 0xFEAD0001;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
      }
      
      // /hollywood/ohci@0d050000
      create_node(/*nProps=*/8, /*nChildren=*/0);
      {
        const char *name = "ohci";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-usb-ohci";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0d050000, 0x00000200
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupts = 5;
        add_property("interrupts", &interrupts, sizeof(interrupts));
        
        u32 interrupt_parent = 0xFEAD0001;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
        
        u16 device_id = 0x1110;
        add_property("device-id", &device_id, sizeof(device_id));
        
        u16 vendor_id = 0x2220;
        add_property("vendor-id", &vendor_id, sizeof(vendor_id));
        
        u8 revision_id = 0x33;
        add_property("revision-id", &revision_id, sizeof(revision_id));
      }
      
      // /hollywood/sdhc@0d070000
      create_node(/*nProps=*/5, /*nChildren=*/0);
      {
        const char *name = "sdhc";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-sdhci";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0d070000, 0x00000200,
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupts = 7;
        add_property("interrupts", &interrupts, sizeof(interrupts));
        
        u32 interrupt_parent = 0xFEAD0001;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
      }
      
      // /hollywood/pic1@0d800030
      create_node(/*nProps=*/8, /*nChildren=*/0);
      {
        const char *name = "pic1";
        add_property("name", name, strlen(name) + 1);
        
        const char *compatible = "nintendo,hollywood-pic";
        add_property("compatible", compatible, strlen(compatible) + 1);
        
        u32 reg[2] = {
          0x0d800030, 0x00000010
        };
        add_property("reg", reg, sizeof(reg));
        
        u32 interrupt_cells = 1;
        add_property("#interrupt-cells", &interrupt_cells, sizeof(interrupt_cells));
        
        u32 phandle = 0xFEAD0001;
        add_property("AAPL,phandle", &phandle, sizeof(phandle));
        
        add_property("interrupt-controller", NULL, 0);
        
        u32 interrupt_parent = 0xFEAD0000;
        add_property("interrupt-parent", &interrupt_parent, sizeof(interrupt_parent));
        
        u32 interrupts = 14;
        add_property("interrupts", &interrupts, sizeof(interrupts));
      }
    }
    
    // /chosen
    create_node(/*nProps=*/1, /*nChildren=*/1);
    {
      const char *name = "chosen";
      add_property("name", name, strlen(name) + 1);
      
      // /chosen/memory-map
      create_node(/*nProps=*/3 + macho_memory_map_entries_count + allocated_drivers_count, /*nChildren=*/0);
      {
        const char* name = "memory-map";
        add_property("name", name, strlen(name) + 1);
        
        u32 boot_args[2] = {
          boot_args_address, BOOT_ARGS_SIZE
        };
        add_property("BootArgs", boot_args, sizeof(boot_args));
        
        u32 framebuffer[2] = {
          0x01600000, 0x00200000
        };
        add_property("framebuffer", framebuffer, sizeof(framebuffer));
        
        for (int i = 0; i < macho_memory_map_entries_count; i++) {
          macho_memory_map_entry_t entry = macho_memory_map_entries[i];
          u32 macho_entry[2] = {
            entry.start, entry.size
          };
          add_property(entry.name, macho_entry, sizeof(macho_entry));
        }
        
        for (int i = 0; i < allocated_drivers_count; i++) {
          driver_info_t *driver_info = allocated_drivers[i];
          u32 address = (u32)driver_info;
          u32 size = sizeof(driver_info_t) + driver_info->info_plist_size + driver_info->bin_size;
          u32 driver[2] = {
            address, size
          };
          char name[15];
          sprintf(name, "Driver-%x", address);
          add_property(name, driver, sizeof(driver));
        }
      }
    }
    
    // /options
    create_node(/*nProps=*/3, /*nChildren=*/0);
    {
      const char *name = "options";
      add_property("name", name, strlen(name) + 1);
      
      boot_args_t *boot_args_ptr = (boot_args_t *)boot_args_address;
      const char *boot_args = boot_args_ptr->CommandLine;
      add_property("boot-args", boot_args, strlen(boot_args) + 1);
      
      const char *boot_command = "boot";
      add_property("boot-command", boot_command, strlen(boot_command) + 1);
    }
    
    // /nvram
    create_node(/*nProps=*/1, /*nChildren=*/0);
    {
      const char *name = "nvram";
      add_property("name", name, strlen(name) + 1);
    }
  }
}

// Helper: round a length up to a multiple of 4 (assuming a 4-byte longword).
static inline u32 pad_length(u32 length) {
  return ((length + 3) / 4) * 4;
}

// This helper function computes the total size (in bytes) of a node,
// including its properties and its children. This is needed so we can
// move from one child node to the next.
u32 get_node_size(DTNode *node) {
  u32 size = sizeof(DTNode);
  // Start at the properties array, right after the DTNode header.
  DTProperty *prop = (DTProperty *)(node + 1);
  for (u32 i = 0; i < node->nProperties; i++) {
    // Each property occupies its header plus the padded property value.
    u32 propSize = sizeof(DTProperty) + pad_length(prop->length);
    size += propSize;
    prop = (DTProperty *)((unsigned char *)prop + propSize);
  }
  // Now, 'prop' points to the children array.
  DTNode *child = (DTNode *)prop;
  for (u32 i = 0; i < node->nChildren; i++) {
    u32 childSize = get_node_size(child);
    size += childSize;
    child = (DTNode *)((unsigned char *)child + childSize);
  }
  return size;
}


// Look for a property by name in a node. Returns pointer if found, otherwise NULL.
DTProperty* find_property(DTNode *node, const char *propName) {
  DTProperty *prop = (DTProperty *)(node + 1);
  for (u32 i = 0; i < node->nProperties; i++) {
    if (strcmp(prop->name, propName) == 0)
      return prop;
    u32 padded = pad_length(prop->length);
    prop = (DTProperty *)((unsigned char *)prop + sizeof(DTProperty) + padded);
  }
  return NULL;
}

// Recursive function that prints a formatted device tree.
void _print_device_tree(DTNode *node, int indent) {
  // Print the current node's header with indentation.
  for (int i = 0; i < indent; i++) {
    printf("  ");
  }
  
  // Try to use the child's "name" property for the path.
  DTProperty *name = find_property(node, "name");
  if (name != NULL && name->length > 0) {
    printf("%s:\n", (char *)(name + 1));
  } else {
    printf("node:\n");
  }
  
  // Process the properties.
  DTProperty *prop = (DTProperty *)(node + 1);
  for (u32 i = 0; i < node->nProperties; i++) {
    // Print property header with extra indentation.
    for (int j = 0; j < indent + 1; j++) {
      printf("  ");
    }
    printf("%s: ", prop->name);
    
    // The property value starts immediately after the DTProperty header.
    unsigned char *val = (unsigned char *)(prop + 1);
    for (u32 k = 0; k < prop->length; k++) {
      printf("%02x ", val[k]);
    }
    printf("\n");
    
    // Move to the next property. We add the size of the header plus the padded length.
    u32 padded = pad_length(prop->length);
    prop = (DTProperty *)((unsigned char *)prop + sizeof(DTProperty) + padded);
  }
  
  // Process the children.
  DTNode *child = (DTNode *)prop;
  for (u32 i = 0; i < node->nChildren; i++) {
    _print_device_tree(child, indent + 1);
    // Advance to the next child using the computed size of the current child.
    child = (DTNode *)((unsigned char *)child + get_node_size(child));
  }
}

void print_device_tree(void *device_tree_ptr) {
  DTNodePtr node = (DTNodePtr)device_tree_ptr;
  _print_device_tree(node, 0);
}
