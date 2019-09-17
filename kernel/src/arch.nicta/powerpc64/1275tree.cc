/****************************************************************************
 *
 * Copyright (C) 2002-2003, Karlsruhe University
 *
 * File path:	arch/powwepc64/1275tree.cc
 * Description:	Functions which enable easy access to the position-independent
 * 		Open Firmware device tree.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $Id: 1275tree.cc,v 1.3 2005/01/18 13:32:35 cvansch Exp $
 *
 ***************************************************************************/

#include INC_ARCH(string.h)
#include INC_ARCH(1275tree.h)

of1275_tree_t of1275_tree;

int of1275_device_t::get_depth()
{
    int depth = 0;
    char *c = this->name;

    /* Special case for "/" root node */
    if ( *c == '/' && *(c+1) == '\0' )
	return 0;

    while( *c )
    {
	if( *c == '/' )
	    depth++;
	c++;
    }
    return depth;
}

static bool dev_is_type(of1275_device_t * dev, const char *type)
{
    char *data;
    u32_t len;
    return (dev->get_prop( "device_type", (char **)&data, &len ) &&
		    !strcmp((const char *)data, type) );
}

static bool dev_is_name(of1275_device_t * dev, const char *type)
{
    char *data;
    u32_t len;
    return (dev->get_prop( "name", (char **)&data, &len ) &&
		    !strcmp((const char *)data, type) );
}

of1275_device_t::device_type_e of1275_device_t::get_type()
{
    device_type_e type = tNONE;

    if ( dev_is_name( this, "device-tree") )
	type = tDEV_TREE;
    else if ( dev_is_type( this, "pci") )
	type = tPCI;
    else if ( dev_is_type( this, "isa") )
	type = tISA;
    else if ( dev_is_type( this, "mac-io") )
	type = tMAC_IO;
    else if ( dev_is_name( this, "u3") )
	type = tU3;

    return type;
}

#include <debug.h>
bool of1275_device_t::get_prop( const char *name, char **data, u32_t *data_len )
{
    of1275_item_t *item_name, *item_data;

    item_name = this->item_first();
    item_data = item_name->next();

    for( word_t i = 0; i < this->get_prop_count(); i++ )
    {
	if( !strcmp(item_name->data, name) )
	{
	    *data = item_data->data;
	    *data_len = item_data->len;
	    return true;
	}
	item_name = item_data->next();
	item_data = item_name->next();
    }

    return false;
}

bool of1275_device_t::get_prop( word_t index, 
	char **name, char **data, u32_t *data_len )
{
    of1275_item_t *item_name, *item_data;

    if( index >= this->get_prop_count() )
	return false;

    item_name = this->item_first();
    item_data = item_name->next();

    for( word_t i = 0; i < index; i++ )
    {
	item_name = item_data->next();
	item_data = item_name->next();
    }

    *name = item_name->data;
    *data = item_data->data;
    *data_len = item_data->len;
    return true;
}

of1275_device_t * of1275_tree_t::find( const char *name )
{
    of1275_device_t *dev = this->first();
    if( !dev )
	return NULL;

    while( dev->is_valid() )
    {
	if( !strcmp_of(dev->get_name(), name) )
	    return dev;
	dev = dev->next();
    }

    return NULL;
}

of1275_device_t * of1275_tree_t::find_handle( u32_t handle )
{
    of1275_device_t *dev = this->first();
    if( !dev )
	return NULL;

    while( dev->is_valid() )
    {
	if( dev->get_handle() == handle )
	    return dev;
	dev = dev->next();
    }

    return NULL;
}

of1275_device_t * of1275_tree_t::get_parent( of1275_device_t *dev )
{
    char *slash = NULL;
    int cnt, depth;

    if( !dev || !this->first() )
	return NULL;

    // Do we have any parents?
    depth = dev->get_depth();
    if( depth < 1 )
	return NULL;

    // Locate the last slash in the name.
    for( char *c = dev->get_name(); *c; c++ )
	if( *c == '/' )
	    slash = c;
    if( slash == NULL )
	return NULL;

    // Count the offset of the last slash.
    cnt = 0;
    for( char *c = dev->get_name(); c != slash; c++ )
	cnt++;

    // Search for the parent node.
    of1275_device_t *parent = this->first();
    while( parent->is_valid() )
    {
	if( !strncmp(parent->get_name(), dev->get_name(), cnt) )
	    if( parent->get_depth() == (depth-1) )
		return parent;
	parent = parent->next();
    }

    return NULL;
}

of1275_device_t * of1275_tree_t::find_device_type( const char *device_type )
{
    of1275_device_t *dev;
    u32_t len;
    char *type;

    dev = this->first();
    if( !dev )
	return NULL;

    while( dev->is_valid() )
    {
	if( dev->get_prop("device_type", &type, &len) )
	    if( !strcmp(type, device_type) )
		return dev;
	dev = dev->next();
    }

    return NULL;
}

of1275_device_t *of1275_device_t::next_by_type( const char *device_type )
{
    of1275_device_t *dev = this;
    u32_t len;
    char *type;

    if( !dev )
	return NULL;
    dev = dev->next();

    while( dev && dev->is_valid() )
    {
	if( dev->get_prop("device_type", &type, &len) )
	    if( !strcmp(type, device_type) )
		return dev;
	dev = dev->next();
    }
    return NULL;
}

static void probe_pci_device( dev_node_t * dev_node )
{
    of1275_pci_assigned_addresses *pci_ptr;
    u32_t len;

    if ( dev_node->dev->get_prop( "assigned-addresses", (char**)&pci_ptr, &len ) &&
		    ( len >= sizeof(of1275_pci_assigned_addresses) ) )
    {
	int i = 0, j = len;
	while ((j -= sizeof(of1275_pci_assigned_addresses)) >= 0) {
	    dev_node->adr[i].space = pci_ptr[i].pci.addr.a_hi;
	    dev_node->adr[i].address =
		    ((u64_t)pci_ptr[i].pci.addr.a_mid << 32) + pci_ptr[i].pci.addr.a_lo;
	    dev_node->adr[i].size =
		    ((u64_t)pci_ptr[i].pci.size_hi << 32) + pci_ptr[i].pci.size_lo;
	    i ++;
	    if (i > 6) {
		printf("too many ranges\n");
		break;
	    }
	}
	dev_node->addrs = i;
    }
}

static void probe_macio_device( dev_node_t * dev_node )
{
    u64_t base_addr = 0;

    dev_node_t * macio = dev_node;

    while ( get_of1275_tree()->get_parent(macio->dev) ) {
	macio = macio - 1;
	if ( dev_is_type( macio->dev, "mac-io") ) {
	    base_addr = macio->adr[0].address;
	}
    }
    
    of1275_reg_property *regs;
    u32_t len;
    if ( dev_node->dev->get_prop( "reg", (char**)&regs, &len ) &&
		    ( (int)len >= 4*(dev_node->addr_cells + dev_node->size_cells) ) )
    {
	int i = 0, j = len;
	while ( (j -= 4*(dev_node->addr_cells + dev_node->size_cells)) >= 0 ) {
	    dev_node->adr[i].space = 2;
	    dev_node->adr[i].address = base_addr +
		    (dev_node->addr_cells > 1 ?
		     ((u64_t)regs[i].val[dev_node->addr_cells-2] << 32) : 0)
		    + regs[i].val[dev_node->addr_cells-1];
	    dev_node->adr[i].size =
		    (dev_node->size_cells > 1 ?
		     ((u64_t)regs[i].val[dev_node->addr_cells + dev_node->size_cells-2] << 32) : 0)
		    + regs[i].val[dev_node->addr_cells + dev_node->size_cells-1];
	    i ++;
	    if (i > 6) {
		printf("too many ranges\n");
		break;
	    }
	}
	dev_node->addrs = i;
    }
}

static void probe_root_device( dev_node_t * dev_node )
{
    of1275_reg_property *regs;
    u32_t len;
    if ( dev_node->dev->get_prop( "reg", (char**)&regs, &len ) &&
		    ( (int)len >= 4*(dev_node->addr_cells + dev_node->size_cells) ) )
    {
	int i = 0, j = len;
	while ( (j -= 4*(dev_node->addr_cells + dev_node->size_cells)) >= 0 ) {
	    dev_node->adr[i].space = 2;
	    dev_node->adr[i].address =
		    (dev_node->addr_cells > 1 ?
		     ((u64_t)regs[i].val[dev_node->addr_cells-2] << 32) : 0)
		    + regs[i].val[dev_node->addr_cells-1];
	    dev_node->adr[i].size =
		    (dev_node->size_cells > 1 ?
		     ((u64_t)regs[i].val[dev_node->addr_cells + dev_node->size_cells-2] << 32) : 0)
		    + regs[i].val[dev_node->addr_cells + dev_node->size_cells-1];
	    i ++;
	    if (i > 6) {
		printf("too many ranges\n");
		break;
	    }
	}
	dev_node->addrs = i;
    }
}

#define MAX_DEPTH	12
dev_node_t dev_list[MAX_DEPTH];

dev_node_t * of1275_tree_t::probe_bus_address( of1275_device_t *dev )
{
    of1275_device_t *node = dev;

    do {
	if (!node || !node->is_valid())
	    return NULL;

	int depth = node->get_depth();
	if (depth >= MAX_DEPTH)
	    return NULL;

	dev_list[depth].dev = node;

	node = get_parent(node);
    } while (node);

    int addr_cells = 1;
    int size_cells = 1;
    of1275_device_t::device_type_e device_type = of1275_device_t::tDEV_TREE;

    for (int i = 0; i <= dev->get_depth(); i++)
    {
	node = dev_list[i].dev;

	dev_list[i].addr_cells = addr_cells;
	dev_list[i].size_cells = size_cells;

	switch (device_type) {
	case of1275_device_t::tDEV_TREE:
	    dev_list[i].addrs = 0;
	    break;
	case of1275_device_t::tPCI:
	    probe_pci_device( &dev_list[i] );
	    break;
	case of1275_device_t::tMAC_IO:
	    probe_macio_device( &dev_list[i] );
	    break;
	case of1275_device_t::tU3:
	    probe_root_device( &dev_list[i] );
	    break;
	default:
	    dev_list[i].addrs = 0;
	    break;
	};

	printf("node %d = %p, (%d, %d, %d), %s\n", node->get_depth(),
			node, device_type, addr_cells, size_cells,
			node->get_name());
	for (int j = 0; j < dev_list[i].addrs; j ++) {
	    printf("  %d: %lx %lx-%lx\n", j,
			    dev_list[i].adr[j].space,
			    dev_list[i].adr[j].address,
			    dev_list[i].adr[j].address + dev_list[i].adr[j].size);
	}

	device_type = node->get_type();

	u32_t * val32;
	u32_t len;

	if (node->get_prop( "#address-cells", (char **)&val32, &len)) {
	    addr_cells = *val32;
	}
	if (node->get_prop( "#size-cells", (char **)&val32, &len)) {
	    size_cells = *val32;
	}
    }

    return &dev_list[dev->get_depth()];
}
