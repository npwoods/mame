// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

        Sun-4 Models
        ------------

    4/260
        Processor(s):   SF9010 @ 16.67MHz, Weitek 1164/1165, Sun-4 MMU,
                        16 hardware contexts
        Speed ratings:  10 MIPS, 1.6 MFLOPS
        CPU:            501-1274/1491/1522
        Chassis type:   deskside
        Bus:            VME (12 slot)
        Memory:         128M (documented) physical with ECC, 1G/process virtual,
                        60ns cycle
        Architecture:   sun4
        Notes:          First SPARC machine. Code-named "Sunrise". Cache
                        much like Sun-3/2xx, uses same memory boards.
                        May be upgraded 3/260.

    4/110
        Processor(s):   MB86900 @ 14.28MHz, Weitek 1164/1165, Sun-4 MMU,
                        16 hardware contexts
        Speed ratings:  7 MIPS
        CPU:            501-1199/1237/1462/1463/1464/1465/1512/1513/
                            1514/1515/1516/1517/1656/1657/1658/1659/
                            1660/1661
        Chassis type:   deskside
        Bus:            VME (3 slot), P4
        Memory:         32M physical with parity, 1G/process virtual,
                        70ns cycle
        Architecture:   sun4
        Notes:          First desktop-able SPARC. CPU doesn't support
                        VME busmaster cards (insufficient room on CPU
                        board for full VME bus interface), so DMA disk
                        and tape boards won't work with it. Originally
                        intended as single-board machine, although there
                        are a few slave-only VME boards (such as the
                        ALM-2 and second ethernet controller) which work
                        with it. SIMM memory (static column?).
                        Code-named "Cobra". CPUs 501-1199/1462/1464/1512/
                        1514/1516/1656/1658/1660 do not have an FPU;
                        501-1237/1463/1465/1513/1515/1517/1657/1659/1661
                        have an FPU.

    4/280
        Chassis type:   rackmount
        Notes:          Rackmount version of 4/260. May be upgraded
                        3/280.

    4/150
        Chassis type:   deskside
        Bus:            VME (6 slot)
        Notes:          See 4/110.

    SPARCstation 1 (4/60)
        Processor(s):   MB86901A or LSI L64801 @ 20MHz, Weitek 3170,
                        Sun-4c MMU, 8 hardware contexts
        Speed ratings:  12.5 MIPS, 1.4 MFLOPS, 10 SPECmark89
        CPU:            501-1382/1629
        Chassis type:   square pizza box
        Bus:            SBus @ 20MHz (3 slots, slot 3 slave-only)
        Memory:         64M physical with synchronous parity,
                        512M/process virtual, 50 ns cycle
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 16-byte lines
        Architecture:   sun4c
        Notes:          Code name "Campus". SIMM memory. 3.5" floppy.
                        First supported in SunOS 4.0.3c.

    SPARCserver 1
        Notes:          SPARCstation 1 without a monitor/framebuffer.

    4/330 (SPARCstation 330, SPARCserver 330)
        Processor(s):   CY7C601 @ 25MHz, TI8847, Sun-4 MMU, 16 hardware
                        contexts
        Speed ratings:  16 MIPS, 2.6 MFLOPS, 11.3 SPECmark89
        CPU:            501-1316/1742
        Bus:            VME (3 9U slots, 2 each 6U and 3U), P4
        Memory:         56M/72M (documented) physical with synchronous
                        parity, 1G/process virtual, 40ns cycle
        Cache:          128K
        Architecture:   sun4
        Notes:          SIMM memory. Cache similar to 4/2xx but
                        write-through. Code-named "Stingray". 56M limit
                        only for early versions of ROM.

    4/310
        Chassis:        deskside
        Bus:            VME (3 slots), P4
        Notes:          See 4/330.

    4/350
        Chassis:        deskside
        Bus:            VME (6 slots), P4
        Notes:          See 4/330.

    4/360
        Notes:          4/260 upgraded with a 4/3xx CPU and memory boards.

    4/370 (SPARCstation 370, SPARCserver 370)
        Chassis:        deskside
        Bus:            VME (12 slots), P4
        Notes:          See 4/330.

    4/380
        Notes:          4/280 upgraded with a 4/3xx CPU and memory boards..

    4/390 (SPARCserver 390)
        Chassis:        rackmount
        Bus:            VME (16 slots)
        Notes:          See 4/330.

    4/470 (SPARCstation 470, SPARCserver 470)
        Processor(s):   CY7C601 @ 33MHz, TI8847 (?), 64 MMU hardware
                        contexts
        Speed ratings:  22 MIPS, 3.8 MFLOPS, 17.6 SPECmark89
        CPU:            501-1381/1899
        Chassis:        deskside
        Bus:            VME (12 slots), P4
        Memory:         96M (documented) physical
        Cache:          128K
        Architecture:   sun4
        Notes:          Write-back rather than write-through cache,
                        3-level rather than 2-level Sun-style MMU.
                        Code-name "Sunray" (which was also the code name
                        for the 7C601 CPU).

    4/490 (SPARCserver 490)
        Chassis:        rackmount
        Bus:            VME (16 slots), P4
        Notes:          See 4/470.

    SPARCstation SLC (4/20)
        Processor(s):   MB86901A or LSI L64801 @ 20MHz
        Speed ratings:  12.5 MIPS, 1.2 MFLOPS, 8.6 SPECmark89
        CPU:            501-1627/1680/1720/1748 (1776/1777 ?)
        Chassis type:   monitor
        Bus:            none
        Memory:         16M physical
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 16-byte lines
        Architecture:   sun4c
        Notes:          Code name "Off-Campus". SIMM memory. No fan.
                        Built into 17" mono monitor. First supported in
                        SunOS 4.0.3c.

    SPARCstation IPC (4/40)
        Processor(s):   MB86901A or LSI L64801 @ 25MHz
        Speed ratings:  13.8 SPECint92, 11.1 SPECfp92, 327
                        SPECintRate92, 263 SPECfpRate92
        CPU:            501-1689/1835/1870/1974 (1690?)
        Chassis type:   lunchbox
        Bus:            SBus @ 25MHz (2 slots)
        Memory:         48M physical
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 16-byte lines
        Architecture:   sun4c
        Notes:          Code name "Phoenix". SIMM memory. Onboard mono
                        framebuffer. 3.5" floppy. First supported in
                        SunOS 4.0.3c.

    SPARCstation 1+ (4/65)
        Processor(s):   LSI L64801 @ 25MHz, Weitek 3172, Sun-4c MMU,
                        8 hardware contexts
        Speed ratings:  15.8 MIPS, 1.7 MFLOPS, 12 SPECmark89
        CPU:            501-1632
        Chassis type:   square pizza box
        Bus:            SBus @ 25MHz (3 slots, slot 3 slave-only)
        Memory:         64M (40M?) physical with synchronous parity,
                        512M/process virtual, 50ns cycle
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 16-byte lines
        Architecture:   sun4c
        Notes:          Code name "Campus B". SIMM memory. 3.5" floppy.
                        Essentially same as SPARCstation 1, just faster
                        clock and improved SCSI controller. First
                        supported in SunOS 4.0.3c.

    SPARCserver 1+
        Notes:          SPARCstation 1+ without a monitor/framebuffer.

    SPARCstation 2 (4/75)
        Processor(s):   CY7C601 @ 40MHz, TI TMS390C601A (602A ?), Sun-4c
                        MMU, 16 hardware contexts
        Speed ratings:  28.5 MIPS, 4.2 MFLOPS, 21.8 SPECint92, 22.8
                        SPECfp92, 517 SPECintRate92, 541 SPECfpRate92
        CPU:            501-1638/1744/1858/1859/1912/1926/1989/1995
        Chassis type:   square pizza box
        Bus:            SBus @ 20MHz (3 slots)
        Memory:         64M physical on motherboard/128M total
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 32-byte lines
        Architecture:   sun4c
        Notes:          Code name "Calvin". SIMMs memory. 3.5" floppy.
                        Case slightly larger and has more ventilation.
                        (Some models apparently have LSI L64811 @
                        40MHz?) Expansion beyond 64M is possible with a
                        32M card which can take a 32M daughterboard
                        (card blocks SBus slot). First supported in
                        SunOS 4.1.1.

    SPARCserver 2
        Notes:          SPARCstation 2 without a monitor/framebuffer.

    SPARCstation ELC (4/25)
        Processor(s):   Fujitsu MB86903 or Weitek W8701 @ 33MHz, FPU on
                        CPU chip, Sun-4c MMU, 8 hardware contexts
        Speed ratings:  21 MIPS, 3 MFLOPS, 18.2 SPECint92, 17.9
                        SPECfp92, 432 SPECintRate92, 425 SPECfpRate92
        CPU:            501-1861 (1730?)
        Chassis type:   monitor
        Bus:            none
        Memory:         64M physical
        Cache:          64K write-through, direct-mapped, virtually
                        indexed, virtually tagged, 32-byte lines
        Architecture:   sun4c
        Notes:          Code name "Node Warrior". SIMM memory. No fan.
                        Built into 17" mono monitor. first supported in
                        SunOS 4.1.1c.

    SPARCstation IPX (4/50)
        Processor(s):   Fujitsu MB86903 or Weitek W8701 @ 40MHz, FPU on
                        CPU chip, Sun-4c MMU, 8 hardware contexts
        Speed ratings:  28.5 MIPS, 4.2 MFLOPS, 21.8 SPECint92,
                        21.5 SPECfp92, 517 SPECintRate92, 510
                        SPECfpRate92
        CPU:            501-1780/1810/1959/2044
        Chassis type:   lunchbox
        Bus:            SBus @ 20MHz (2 slots)
        Memory:         64M physical
        Cache:          64K write-through cache, direct-mapped,
                        virtually indexed, virtually tagged, 32-byte
                        lines
        Architecture:   sun4c
        Notes:          Code name "Hobbes". SIMM memory. Onboard
                        GX-accelerated cg6 color framebuffer (not usable
                        with ECL mono monitors, unlike SBus version).
                        Picture of Hobbes (from Watterson's "Calvin and
                        Hobbes" comic strip) silkscreened on
                        motherboard. 3.5" floppy. First supported in
                        SunOS 4.1.1 (may require IPX supplement).

    SPARCengine 1 (4/E)
        CPU:            501-8035/8058/8064
        Bus:            VME (6U form factor), SBus (1 slot)
        Notes:          Single-board VME SPARCstation 1 (or 1+?),
                        presumably for use as a controller, not as a
                        workstation. 8K MMU pages rather than 4K.
                        External RAM, framebuffer, and SCSI/ethernet
                        boards available. Code name "Polaris".

    SPARCserver 630MP (4/630)
        Processor(s):   MBus modules
        CPU:            501-1686/2055
        Chassis type:   deskside
        Bus:            VME (3 9U slots, 2 each 6U and 3U), SBus @ 20MHz
                        (4 slots), MBus (2 slots)
        Memory:         640M physical
        Architecture:   sun4m
        Notes:          First MBus-based machine. Code name "Galaxy".
                        SIMM memory.

    SPARCserver 670MP (4/670)
        Chassis type:   deskside
        Bus:            VME (12 slots), SBus @ 20MHz (4 slots), MBus (2
                        slots)
        Notes:          Like SPARCserver 630MP. More SBus slots can be
                        added via VME expansion boards.

    SPARCserver 690MP (4/690)
        Chassis type:   rackmount
        Bus:            VME (16 slots), SBus @ 20MHz (4 slots), MBus (2
                        slots)
        Notes:          See SPARCserver 670MP.

    SPARCclassic (SPARCclassic Server)(SPARCstation LC) (4/15)
        Processor(s):   microSPARC @ 50MHz
        Speed ratings:  59.1 MIPS, 4.6 MFLOPS, 26.4 SPECint92, 21.0
                        SPECfp92, 626 SPECintRate92, 498 SPECfpRate92
        CPU:            501-2200/2262/2326
        Chassis type:   lunchbox
        Bus:            SBus @ 20MHz (2 slots)
        Memory:         96M physical
        Architecture:   sun4m
        Notes:          Sun4m architecture, but no MBus (uniprocessor
                        only). SIMM memory. Shares code name "Sunergy"
                        with LX. 3.5" floppy. Soldered CPU chip. Onboard
                        cgthree framebuffer, AMD79C30 8-bit audio chip.
                        First supported in SunOS 4.1.3c.

    SPARCclassic X (4/10)
        CPU:            501-2079/2262/2313
        Notes:          Essentially the same as SPARCclassic, but
                        intended for use as an X terminal (?).

    SPARCstation LX/ZX (4/30)
        Processor(s):   microSPARC @ 50MHz
        Speed ratings:  59.1 MIPS, 4.6 MFLOPS, 26.4 SPECint92, 21.0
                        SPECfp92, 626 SPECintRate92, 498 SPECfpRate92
        CPU:            501-2031/2032/2233/2474
        Chassis type:   lunchbox
        Bus:            SBus @ 20MHz (2 slots)
        Memory:         96M physical
        Architecture:   sun4m
        Notes:          Sun4m architecture, but no MBus (uniprocessor
                        only). SIMM memory. Shares code name "Sunergy"
                        with SPARCclassic. Soldered CPU chip. Onboard
                        cgsix framebuffer, 1M VRAM standard, expandable
                        to 2M. DBRI 16-bit audio/ISDN chip. First
                        supported in SunOS 4.1.3c.

   SPARCstation Voyager
        Processors(s):  microSPARC II @ 60MHz
        Speed ratings:  47.5 SPECint92, 40.3 SPECfp92, 1025
                        SPECintRate92, 859 SPECfpRate92
        Bus:            SBus; PCMCIA type II (2 slots)
        Memory:         80M physical
        Architecture:   sun4m
        Notes:          Portable (laptop?). 16M standard, two memory
                        expansion slots for Voyager-specific SIMMs (16M
                        or 32M). Code-named "Gypsy". 14" 1152x900 mono
                        or 12" 1024x768 color flat panel displays. DBRI
                        16-bit audio/ISDN chip.

    SPARCstation 3
        Notes:          Although this model appeared in a few Sun price
                        lists, it was renamed the SPARCstation 10 before
                        release.

    SPARCstation 10/xx
        Processor(s):   MBus modules
        Motherboard:    501-1733/2259/2274/2365 (-2274 in model 20 only)
        Chassis type:   square pizza box
        Bus:            SBus @ 16.6/20MHz (model 20) or 18/20MHz (other
                        models) (4 slots); MBus (2 slots)
        Memory:         512M physical
        Architecture:   sun4m
        Notes:          Code name "Campus-2". 3.5" floppy. SIMM memory.
                        Some models use double-width MBus modules which
                        block SBus slots. Also, the inner surface of the
                        chassis is conductive, so internal disk drives
                        must be mounted with insulating hardware.

    SPARCserver 10/xx
        Notes:          SPARCstation 10/xx without monitor/framebuffer.

    SPARCcenter 2000
        Processor(s):   MBus modules
        Motherboard:    501-1866/2334/2362
        Bus:            XDBus * 2 (20 slots); SBus @ 20MHz (4
                        slots/motherboard); MBus (2 slots/motherboard)
        Memory:         5G physical
        Cache:          2M/motherboard
        Architecture:   sun4d
        Notes:          Dual XDBus backplane with 20 slots. One board
                        type that carries dual MBus modules with 2M
                        cache (1M for each XDBus), 512M memory and 4
                        SBus slots. Any combination can be used; memory
                        is *not* tied to the CPU modules but to an
                        XDBus. Solaris 2.x releases support an
                        increasing number of CPUs (up to twenty), due to
                        tuning efforts in the kernel. First supported in
                        Solaris 2.2 (SunOS 5.2). Code name "Dragon".

    SPARCserver 1000
        Processor(s):   MBus modules
        Motherboard:    501-2336 (2338?)
        Bus:            XDBus; SBus @ 20MHz (3 slots/motherboard); MBus
                        (2 slots/motherboard)
        Memory:         2G physical
        Cache:          1M/motherboard
        Architecture:   sun4d
        Notes:          Single XDBus design with "curious L-shaped
                        motherboards". Three SBus slots per motherboard,
                        512M, two MBus modules per motherboard. Four
                        motherboards total, or a disk tray with four 1"
                        high 3.5" disks. Code name "Scorpion". First
                        supported in Solaris 2.2 (SunOS 5.2).



        21/11/2011 Skeleton driver.

****************************************************************************/

#include "emu.h"
#include "cpu/sparc/sparc.h"
#include "machine/z80scc.h"
#include "bus/rs232/rs232.h"

#define ENA_NOTBOOT		0x80
#define ENA_SDVMA		0x20
#define ENA_CACHE		0x10
#define ENA_RESET		0x04
#define ENA_DIAG		0x01

#define SCC1_TAG		"kbdmouse"
#define SCC2_TAG		"uart"
#define RS232A_TAG      "rs232a"
#define RS232B_TAG      "rs232b"

#define ASI_SYSTEM_SPACE	2
#define ASI_SEGMENT_MAP		3
#define ASI_PAGE_MAP		4
#define ASI_USER_INSN		8
#define ASI_SUPER_INSN		9
#define ASI_USER_DATA		10
#define ASI_SUPER_DATA		11
#define ASI_FLUSH_SEGMENT	12
#define ASI_FLUSH_PAGE		13
#define ASI_FLUSH_CONTEXT	14

class sun4_state : public driver_device
{
public:
	sun4_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		, m_rom(*this, "user1")
		, m_kbdmouse(*this, SCC1_TAG)
		, m_uart(*this, SCC2_TAG)
		, m_rom_ptr(nullptr)
		, m_context(0)
		, m_system_enable(0)
	{
	}

	virtual void machine_reset() override;
	virtual void machine_start() override;

	DECLARE_READ32_MEMBER( sun4_mmu_r );
	DECLARE_WRITE32_MEMBER( sun4_mmu_w );

protected:

	required_device<mb86901_device> m_maincpu;
	required_memory_region m_rom;
	required_device<z80scc_device> m_kbdmouse;
	required_device<z80scc_device> m_uart;

	UINT32 read_supervisor_data(UINT32 vaddr, UINT32 mem_mask);
	void write_supervisor_data(UINT32 vaddr, UINT32 data, UINT32 mem_mask);

	UINT32 *m_rom_ptr;
	UINT32 m_context;
	UINT16 m_segmap[8][4096];
	UINT32 m_pagemap[8192];
	UINT32 m_system_enable;
};

#define SEGMENT(vaddr)	m_segmap[m_context & 7][((vaddr) >> 18) & 0xfff]
#define PAGE(vaddr)		m_pagemap[((m_segmap[m_context & 7][((vaddr) >> 18) & 0xfff] & 0x7f) << 6) | (((vaddr) >> 12) & 0x3f)]
#define PADDR(vaddr)	(((PAGE(vaddr) << 12) & 0x0ffff000) | ((vaddr) & 0xfff))

UINT32 sun4_state::read_supervisor_data(UINT32 vaddr, UINT32 mem_mask)
{
	UINT32 page = PAGE(vaddr);
	bool v = (page & 0x80000000) ? true : false;
	bool w = (page & 0x40000000) ? true : false;
	bool s = (page & 0x20000000) ? true : false;
	bool x = (page & 0x10000000) ? true : false;
	 int t = (page & 0x0c000000) >> 26;
	bool a = (page & 0x02000000) ? true : false;
	bool m = (page & 0x01000000) ? true : false;
	char mode[4] = { 'M', 'S', '0', '1' };

	logerror("supervisor data read: vaddr %08x, paddr %08x, context %d, segment entry %02x, page entry %08x, %c%c%c%c%c%c%c\n", vaddr, PADDR(vaddr), m_context, SEGMENT(vaddr), PAGE(vaddr)
		, v ? 'V' : 'v', w ? 'W' : 'w', s ? 'S' : 's', x ? 'X' : 'x', mode[t], a ? 'A' : 'a', m ? 'M' : 'm');
	return 0;
}

void sun4_state::write_supervisor_data(UINT32 vaddr, UINT32 data, UINT32 mem_mask)
{
	UINT32 page = PAGE(vaddr);
	bool v = (page & 0x80000000) ? true : false;
	bool w = (page & 0x40000000) ? true : false;
	bool s = (page & 0x20000000) ? true : false;
	bool x = (page & 0x10000000) ? true : false;
	 int t = (page & 0x0c000000) >> 26;
	bool a = (page & 0x02000000) ? true : false;
	bool m = (page & 0x01000000) ? true : false;
	char mode[4] = { 'M', 'S', '0', '1' };

	logerror("supervisor data write: vaddr %08x, paddr %08x, data %08x, mem_mask %08x, context %d, segment entry %02x, page entry %08x, %c%c%c%c%c%c%c\n", vaddr, PADDR(vaddr), data, mem_mask,
		m_context, SEGMENT(vaddr), PAGE(vaddr), v ? 'V' : 'v', w ? 'W' : 'w', s ? 'S' : 's', x ? 'X' : 'x', mode[t], a ? 'A' : 'a', m ? 'M' : 'm');
}

READ32_MEMBER( sun4_state::sun4_mmu_r )
{
	UINT8 asi = m_maincpu->get_asi();

	if (!space.debugger_access())
	{
		switch(asi)
		{
			case ASI_SYSTEM_SPACE:
				switch (offset >> 26)
				{
					case 3: // context reg
						logerror("sun4: read context register %08x (& %08x) asi 2, offset %x, PC = %x\n", m_context << 24, mem_mask, offset << 2, m_maincpu->pc());
						return m_context << 24;

					case 4: // system enable reg
						logerror("sun4: read system enable register %08x (& %08x) asi 2, offset %x, PC = %x\n", m_system_enable << 24, mem_mask, offset << 2, m_maincpu->pc());
						return m_system_enable << 24;

					case 8: // (d-)cache tags
						logerror("sun4: read dcache tags %08x (& %08x) asi 2, offset %x, PC = %x\n", 0xffffffff, mem_mask, offset << 2, m_maincpu->pc());
						return 0xffffffff;

					case 9: // (d-)cache data
						logerror("sun4: read dcache data %08x (& %08x) asi 2, offset %x, PC = %x\n", 0xffffffff, mem_mask, offset << 2, m_maincpu->pc());
						return 0xffffffff;

					case 15: // Type 1 space passthrough
						switch ((offset >> 22) & 15)
						{
							case 0: // keyboard/mouse
								switch (offset & 1)
								{
									case 0:
									{
										UINT32 ret = 0;
										if (mem_mask & 0xffff0000)
											ret |= m_kbdmouse->cb_r(space, 0) << 24;
										if (mem_mask & 0x0000ffff)
											ret |= m_kbdmouse->db_r(space, 0) << 8;
										return ret;
									}
									case 1:
									{
										UINT32 ret = 0;
										if (mem_mask & 0xffff0000)
											ret |= m_kbdmouse->ca_r(space, 0) << 24;
										if (mem_mask & 0x0000ffff)
											ret |= m_kbdmouse->da_r(space, 0) << 8;
										return ret;
									}
								}
								break;
							case 1: // serial ports
								switch (offset & 1)
								{
									case 0:
									{
										UINT32 ret = 0;
										if (mem_mask & 0xffff0000)
											ret |= m_uart->cb_r(space, 0) << 24;
										if (mem_mask & 0x0000ffff)
											ret |= m_uart->db_r(space, 0) << 8;
										return ret;
									}
									case 1:
									{
										UINT32 ret = 0;
										if (mem_mask & 0xffff0000)
											ret |= m_uart->ca_r(space, 0) << 24;
										if (mem_mask & 0x0000ffff)
											ret |= m_uart->da_r(space, 0) << 8;
										return ret;
									}
								}
								break;

							default:
								logerror("sun4: read unknown type 1 space at address %x (& %08x) asi 2, offset %x, PC = %x\n", offset << 2, mem_mask, offset << 2, m_maincpu->pc());
						}
						break;

					case 0: // IDPROM - TODO: SPARCstation-1 does not have an ID prom and a timeout should occur.
					default:
						logerror("sun4: read unknown register (& %08x) asi 2, offset %x, PC = %x\n", mem_mask, offset << 2, m_maincpu->pc());
						return 0;
				}
				break;

			case ASI_SEGMENT_MAP:
			{
				logerror("sun4: read m_segmap[%d][(%08x >> 18) & 0xfff = %03x] = %08x << 24 & %08x\n", m_context & 7, offset << 2, (offset >> 16) & 0xfff, SEGMENT(offset << 2), mem_mask);
				UINT32 result = SEGMENT(offset << 2);
				while (!(mem_mask & 1))
				{
					mem_mask >>= 1;
					result <<= 1;
				}
				return result;
			}

			case ASI_PAGE_MAP: // page map
			{
				logerror("sun4: read m_pagemap[(m_segmap[%d][(%08x >> 18) & 0xfff = %03x] = %08x << 6) | ((%08x >> 12) & 0x3f)]] = %08x & %08x\n", m_context & 7, offset << 2, (offset >> 16) & 0xfff, SEGMENT(offset << 2), offset << 2, PAGE(offset << 2), mem_mask);
				return PAGE(offset << 2);
			}

			case ASI_SUPER_INSN: // supervisor instruction space
				return m_rom_ptr[offset & 0x1ffff]; // wrong, but works for now

			case ASI_SUPER_DATA:
				return read_supervisor_data(offset << 2, mem_mask);

			default:
				logerror("sun4: read (& %08x) asi %d byte offset %x, PC = %x\n", mem_mask, asi, offset << 2, m_maincpu->pc());
				break;
		}
	}

	if (!(m_system_enable & ENA_NOTBOOT))
	{
		return m_rom_ptr[offset & 0x1ffff];
	}

	return 0;
}

WRITE32_MEMBER( sun4_state::sun4_mmu_w )
{
	UINT8 asi = m_maincpu->get_asi();

	switch (asi)
	{
		case 2:
			switch (offset >> 26)
			{
				case 3: // context reg
					logerror("sun4: %08x (& %08x) asi 2 to context register, offset %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					m_context = (UINT8)(data >> 24) & 7;
					return;

				case 4: // system enable reg
					logerror("sun4: write %08x (& %08x) asi 2 to system enable register, offset %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					m_system_enable = (UINT8)data;
					return;

				case 8: // cache tags
					logerror("sun4: write %08x (& %08x) asi 2 to cache tags @ %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					return;

				case 9: // cache data
					logerror("sun4: write %08x (& %08x) asi 2 to cache data @ %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					return;

				case 15: // Type 1 space passthrough
					switch ((offset >> 22) & 15)
					{
						case 0: // keyboard/mouse
							switch (offset & 1)
							{
								case 0:
									if (mem_mask & 0xffff0000)
										m_kbdmouse->cb_w(space, 0, data >> 24);
									if (mem_mask & 0x0000ffff)
										m_kbdmouse->db_w(space, 0, data >> 24);
									break;
								case 1:
									if (mem_mask & 0xffff0000)
										m_kbdmouse->ca_w(space, 0, data >> 24);
									if (mem_mask & 0x0000ffff)
										m_kbdmouse->da_w(space, 0, data >> 24);
									break;
							}
							break;
						case 1: // serial ports
							switch (offset & 1)
							{
								case 0:
									if (mem_mask & 0xffff0000)
										m_uart->cb_w(space, 0, data >> 24);
									if (mem_mask & 0x0000ffff)
										m_uart->db_w(space, 0, data >> 24);
									break;
								case 1:
									if (mem_mask & 0xffff0000)
										m_uart->ca_w(space, 0, data >> 24);
									if (mem_mask & 0x0000ffff)
										m_uart->da_w(space, 0, data >> 24);
									break;
							}
							break;
						default:
							logerror("sun4: write unknown type 1 space %08x (& %08x) asi 2, offset %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					}
					break;

				case 0: // IDPROM
				default:
					logerror("sun4: write %08x (& %08x) asi 2 to unknown register, offset %x, PC = %x\n", data, mem_mask, offset << 2, m_maincpu->pc());
					return;
			}
			break;

		case 3: // segment map
			offset <<= 2;
			while (!(mem_mask & 1))
			{
				mem_mask >>= 1;
				data >>= 1;
			}
			SEGMENT(offset) = data;
			//m_segmap[m_context & 7][(offset >> 18) & 0xfff] &= ~(mem_mask >> 16);
			//m_segmap[m_context & 7][(offset >> 18) & 0xfff] |= (data >> 16) & (mem_mask >> 16);
			logerror("sun4: write m_segmap[%d][(%08x >> 18) & 0xfff = %03x] = %08x & %08x\n", m_context & 7, offset << 2, (offset >> 16) & 0xfff, data, mem_mask);
			break;

		case ASI_PAGE_MAP: // page map
		{
			logerror("sun4: write m_pagemap[(m_segmap[%d][(%08x >> 18) & 0xfff = %03x] = %08x << 6) | ((%08x >> 12) & 0x3f)]] = %08x & %08x\n", m_context & 7, offset << 2,
				(offset >> 16) & 0xfff, SEGMENT(offset << 2), offset << 2, data, mem_mask);
			COMBINE_DATA(&PAGE(offset << 2));
			PAGE(offset << 2) &= 0xff00ffff;
			break;
		}

		case ASI_SUPER_DATA:
			write_supervisor_data(offset << 2, data, mem_mask);
			break;

		default:
			logerror("sun4: write %08x (& %08x) to asi %d byte offset %x, PC = %x\n", data, mem_mask, asi, offset << 2, m_maincpu->pc());
			break;
	}
}

static ADDRESS_MAP_START(sun4_mem, AS_PROGRAM, 32, sun4_state)
	AM_RANGE(0x00000000, 0xffffffff) AM_READWRITE( sun4_mmu_r, sun4_mmu_w )
ADDRESS_MAP_END

/* Input ports */
static INPUT_PORTS_START( sun4 )
INPUT_PORTS_END


void sun4_state::machine_reset()
{
}

void sun4_state::machine_start()
{
	m_rom_ptr = (UINT32 *)m_rom->base();
}

static MACHINE_CONFIG_START( sun4, sun4_state )
	/* basic machine hardware */
	MCFG_CPU_ADD("maincpu", MB86901, 16670000)
	MCFG_DEVICE_ADDRESS_MAP(AS_PROGRAM, sun4_mem)

	MCFG_SCC8530_ADD(SCC1_TAG, XTAL_4_9152MHz, 0, 0, 0, 0)
	MCFG_SCC8530_ADD(SCC2_TAG, XTAL_4_9152MHz, 0, 0, 0, 0)
	MCFG_Z80SCC_OUT_TXDA_CB(DEVWRITELINE(RS232A_TAG, rs232_port_device, write_txd))
	MCFG_Z80SCC_OUT_TXDB_CB(DEVWRITELINE(RS232B_TAG, rs232_port_device, write_txd))

	MCFG_RS232_PORT_ADD(RS232A_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, rxa_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, dcda_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, ctsa_w))

	MCFG_RS232_PORT_ADD(RS232B_TAG, default_rs232_devices, nullptr)
	MCFG_RS232_RXD_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, rxb_w))
	MCFG_RS232_DCD_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, dcdb_w))
	MCFG_RS232_CTS_HANDLER(DEVWRITELINE(SCC2_TAG, z80scc_device, ctsb_w))
MACHINE_CONFIG_END

/*
Boot PROM

Sun-4c Architecture

SPARCstation SLC (Sun-4/20) - 128K x 8
U1001       Revision
========================================
520-2748-01 1.2 Version 3
520-2748-02 1.3
520-2748-03 1.3
520-2748-04 1.4 Version 2
595-2250-xx Sun-4/20 Boot PROM Kit

SPARCstation ELC (Sun-4/25) - 256K x 8
U0806       Revision
========================================
520-3085-01
520-3085-02 2.3 Version 95
520-3085-03 2.4 Version 96
520-3085-04 2.6 Version 102 (not used)
520-3085-04 2.9 Version 7


SPARCstation IPC (Sun-4/40) - 256K x 8
U0902       Revision
========================================
525-1085-01
525-1085-02
525-1085-03 1.6 Version 151
525-1191-01 1.7 Version 3 and 2.4 Version 362
525-1191-02 1.7 Version 3 and 2.6 Version 411
525-1191-03 1.7 Version 3 and 2.9 Version 24


SPARCstation IPX (Sun-4/50) - 256K x 8
U0501       Revision
========================================
525-1177-01 2.1 Version 66
525-1177-02 2.2 Version 134
525-1177-03 2.3 Version 263
525-1177-04 2.4 Version 347
525-1177-05 2.6 Version 410
525-1177-06 2.9 Version 20


SPARCstation 1 (Sun-4/60) - 128K x 8
U0837       Revision
========================================
525-1043-01
525-1043-02 0.1
525-1043-03
525-1043-04 1.0
525-1043-05 1.0
525-1043-06 1.0
525-1043-07 1.1
525-1043-08 1.3 Version 3
595-1963-xx Sun-4/60 Boot PROM Kit
525-1207-01 2.4 Version 95
525-1207-02 2.9 Version 9 (2.x is only available from the spare parts price list)
560-1805-xx Sun-4/60 2.4 Boot PROM Kit


SPARCstation 1+ (Sun-4/65) - 128K x 8
U0837 Revision
========================================
525-1108-01
525-1108-02
525-1108-03 1.1 Version 13
525-1108-04 1.2
525-1108-05 1.3 Version 4
525-1208-01 2.4 Version 116
525-1208-02 2.9 Version 9 (2.x is only available from the spare parts price list)
560-1806-xx Sun-4/65 2.4 Boot PROM Kit


SPARCstation 2 (Sun-4/75) - 256K x 8
U0501       Revision
========================================
525-1107-01 2.0Beta0
525-1107-02 2.0Beta1
525-1107-03 2.0
525-1107-04 2.0 Version 865 (fails with Weitek Power ?p)
525-1107-05 2.1 Version 931 (fails with Weitek Power ?p)
525-1107-06 2.2 Version 947
525-1107-07 2.4 Version 990
525-1107-08 2.4.1 Version 991
525-1107-09 2.6 Version 1118
525-1107-10 2.9 Version 16
595-2249-xx Sun-4/75 Boot PROM Kit

*/

// Sun 4/300, Cypress Semiconductor CY7C601, Texas Instruments 8847 FPU
ROM_START( sun4_300 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "1035-09.rom", 0x0000, 0x10000, CRC(4ae2f2ad) SHA1(9c17a80b3ce3efdf18b5eca969f1565ddaad3116))
	ROM_LOAD( "1036-09.rom", 0x0000, 0x10000, CRC(cb3d45a7) SHA1(9d5da09ff87ec52dc99ffabd1003d30811eafdb0))
	ROM_LOAD( "1037-09.rom", 0x0000, 0x10000, CRC(4f005bea) SHA1(db3f6133ea7c497ba440bc797123dde41abea6fd))
	ROM_LOAD( "1038-09.rom", 0x0000, 0x10000, CRC(1e429d31) SHA1(498ce4d34a74ea6e3e369bb7eb9c2b87e12bd080))

	ROM_REGION( 0x10000, "devices", ROMREGION_ERASEFF )
	// CG3 Color frame buffer (cgthree)
	ROM_LOAD( "sunw,501-1415.bin", 0x0000, 0x0800, CRC(d1eb6f4d) SHA1(9bef98b2784b6e70167337bb27cd07952b348b5a))

	// BW2 frame buffer (bwtwo)
	ROM_LOAD( "sunw,501-1561.bin", 0x0800, 0x0800, CRC(e37a3314) SHA1(78761bd2369cb0c58ef1344c697a47d3a659d4bc))

	// TurboGX 8-Bit Color Frame Buffer
	ROM_LOAD( "sunw,501-2325.bin", 0x1000, 0x8000, CRC(bbdc45f8) SHA1(e4a51d78e199cd57f2fcb9d45b25dfae2bd537e4))
ROM_END

// SPARCstation IPC (Sun 4/40)
ROM_START( sun4_40 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "4.40_v2.9.rom", 0x0000, 0x40000, CRC(532fc20d) SHA1(d86d9e958017b3fecdf510d728a3e46a0ce3281d))
ROM_END

// SPARCstation IPX (Sun 4/50)
ROM_START( sun4_50 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "ipx-29.rom", 0x0000, 0x40000, CRC(1910aa65) SHA1(7d8832fea8e299b89e6ec7137fcde497673c14f8))
ROM_END

// SPARCstation SLC (Sun 4/20)
ROM_START( sun4_20 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "520-2748-04.rom", 0x0000, 0x20000, CRC(e85b3fd8) SHA1(4cbc088f589375e2d5983f481f7d4261a408702e))
ROM_END

// SPARCstation 1 (Sun 4/60)
ROM_START( sun4_60 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "ss1v29.rom", 0x0000, 0x20000, CRC(e3f103a9) SHA1(5e95835f1090ea94859bd005757f0e7b5e86181b))
ROM_END

// SPARCstation 2 (Sun 4/75)
ROM_START( sun4_75 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_LOAD( "ss2-29.rom", 0x0000, 0x40000, CRC(d04132b3) SHA1(ef26afafa2800b8e2e5e994b3a76ca17ce1314b1))
ROM_END

// SPARCstation 10 (Sun S10)
ROM_START( sun_s10 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "r225", "Rev 2.2.5")
	ROMX_LOAD( "ss10_v2.25.rom", 0x0000, 0x80000, CRC(c7a48fd3) SHA1(db13d85b02f181eb7fce4c38b11996ff64116619), ROM_BIOS(1))
	// SPARCstation 10 and 20
	ROM_SYSTEM_BIOS(1, "r225r", "Rev 2.2.5r")
	ROMX_LOAD( "ss10-20_v2.25r.rom", 0x0000, 0x80000, CRC(105ba132) SHA1(58530e88369d1d26ab11475c7884205f2299d255), ROM_BIOS(2))
ROM_END

// SPARCstation 20
ROM_START( sun_s20 )
	ROM_REGION32_BE( 0x80000, "user1", ROMREGION_ERASEFF )
	ROM_SYSTEM_BIOS(0, "r225", "Rev 2.2.5")
	ROMX_LOAD( "ss20_v2.25.rom", 0x0000, 0x80000, CRC(b4f5c547) SHA1(ee78312069522094950884d5bcb21f691eb6f31e), ROM_BIOS(1))
	// SPARCstation 10 and 20
	ROM_SYSTEM_BIOS(1, "r225r", "Rev 2.2.5r")
	ROMX_LOAD( "ss10-20_v2.25r.rom", 0x0000, 0x80000, CRC(105ba132) SHA1(58530e88369d1d26ab11475c7884205f2299d255), ROM_BIOS(2))
ROM_END

/* Driver */

/*    YEAR  NAME    PARENT  COMPAT   MACHINE    INPUT    INIT    COMPANY         FULLNAME       FLAGS */
COMP( 198?, sun4_300,  0,       0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "Sun 4/3x0", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun4_40,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation IPC (Sun 4/40)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun4_50,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation IPX (Sun 4/50)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun4_20,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation SLC (Sun 4/20)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun4_60,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation 1 (Sun 4/60)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun4_75,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation 2 (Sun 4/75)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun_s10,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation 10 (Sun S10)", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
COMP( 198?, sun_s20,   sun4_300,0,       sun4,      sun4, driver_device,     0,  "Sun Microsystems", "SPARCstation 20", MACHINE_NOT_WORKING | MACHINE_NO_SOUND)
