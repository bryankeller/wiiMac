//
// Created by Bryan Keller on 3/31/25.
//

#include "boot_args.h"
#include "bootmii_ppc.h"
#include "device_tree.h"
#include "macho_loader.h"
#include "string.h"
#include "types.h"

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
    create_node(/*nProps=*/4, /*nChildren=*/5);
    {
      	const char *name = "device-tree";
        add_property("name", name, strlen(name) + 1);

        const char *compatible = "nintendo,wii";
        add_property("compatible", compatible, strlen(compatible) + 1);

        u32 address_cells = 1;
        add_property("#address-cells", &address_cells, sizeof(address_cells));

        u32 size_cells = 1;
        add_property("#size-cells", &size_cells, sizeof(size_cells));

        // /options
        create_node(/*nProps=*/3, /*nChildren=*/0);
        {
      	    const char *name = "options";
      	    add_property("name", name, strlen(name) + 1);

      	    const char *boot_args = boot_args_command_line;
      	    add_property("boot-args", boot_args, strlen(boot_args) + 1);

      	    const char *boot_command = "boot";
      	    add_property("boot-command", boot_command, strlen(boot_command) + 1);
        }

        // /chosen
        create_node(/*nProps=*/1, /*nChildren=*/1);
        {
            const char *name = "chosen";
            add_property("name", name, strlen(name) + 1);

      	    // /chosen/memory-map
      	    create_node(/*nProps=*/7, /*nChildren=*/0);
      	    {
                const char* name = "memory-map";
                add_property("name", name, strlen(name) + 1);

                u32 kernel_header[2] = {
                  kernel_header_start, kernel_header_size
                };
                add_property("Kernel-__HEADER", kernel_header, sizeof(kernel_header));
              
                u32 kernel_vectors[2] = {
                    0x00000000, 0x00011000
                };
                add_property("Kernel-__VECTORS", kernel_vectors, sizeof(kernel_vectors));

                u32 kernel_text[2] = {
                    kernel_text_start, kernel_text_size
                };
                add_property("Kernel-__TEXT", kernel_text, sizeof(kernel_text));

                u32 kernel_data[2] = {
                    kernel_data_start, kernel_data_size
                };
                add_property("Kernel-__DATA", kernel_data, sizeof(kernel_data));

                u32 kernel_symtab[2] = {
                    kernel_symtab_start, kernel_symtab_size
                };
                add_property("Kernel-__SYMTAB", kernel_symtab, sizeof(kernel_symtab));

                u32 boot_args[2] = {
                    boot_args_address, boot_args_size
                };
                add_property("BootArgs", boot_args, sizeof(boot_args));
            }
        }

        // /memory
        create_node(/*nProps=*/3, /*nChildren=*/0);
        {
            const char *name = "memory";
            add_property("name", name, strlen(name) + 1);

            const char *device_type = "memory";
            add_property("device_type", device_type, strlen(device_type) + 1);

            u32 reg[4] = {
                0x00000000, 0x01800000, // 24MB MEM1
                0x10000000, 0x03400000  // 58MB MEM2 (upper 12MB used for IOS?)
            };
            add_property("reg", reg, sizeof(reg));
        }

        // /cpus
        create_node(/*nProps=*/1, /*nChildren=*/1);
        {
            const char *name = "cpus";
            add_property("name", name, strlen(name) + 1);

            // /cpus/PowerPC,750
            create_node(/*nProps=*/7, /*nChildren=*/0);
            {
                const char *name = "PowerPC,750";
                add_property("name", name, strlen(name) + 1);

                u32 address_cells = 1;
                add_property("#address-cells", &address_cells, sizeof(address_cells));

                u32 size_cells = 0;
                add_property("#size-cells", &size_cells, sizeof(size_cells));

                const char *dtype = "cpu";
                add_property("device_type", dtype, strlen(dtype) + 1);

                u32 clockFreq = 729000000; // Wii's Broadway CPU ~729MHz
                add_property("clock-frequency", &clockFreq, sizeof(clockFreq));

                u32 tbFreq = 60750000; // 243MHz / 4
                add_property("timebase-frequency", &tbFreq, sizeof(tbFreq));

                u32 busFreq = 243000000; // 729000000 / 3
                add_property("bus-frequency", &busFreq, sizeof(busFreq));
            }
        }

        // /hollywood@0c000000
        create_node(/*nProps=*/7, /*nChildren=*/1);
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
                0x0d800000, 0x0d800000, 0x00800000
            };
            add_property("ranges", ranges, sizeof(ranges));

            u32 reg[] = {
                0x0c000000, 0x01000000,
                0x0d000000, 0x00800000,
                0x0d800000, 0x00800000
            };
            add_property("reg", reg, sizeof(reg));

            // /hollywood@0c000000/sd@0d070000
            create_node(/*nProps=*/4, /*nChildren=*/0);
            {
				const char *name = "sd";
        		add_property("name", name, strlen(name) + 1);

                const char *device_type = "sd";
                add_property("device_type", device_type, strlen(device_type) + 1);

                const char *compatible = "nintendo,hollywood-sdhci";
        		add_property("compatible", compatible, strlen(compatible) + 1);

                u32 reg[2] = {
                	0x0d070000, 0x00000200
            	};
                add_property("reg", reg, sizeof(reg));
            }
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
