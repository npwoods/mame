// license:BSD-3-Clause
// copyright-holders:
/***********************************************************************************************************************************

2017-11-17 Skeleton

A selection of terminals from Bitsavers. A placeholder for now.

************************************************************************************************************************************/

#include "emu.h"
#include "cpu/z80/z80.h"

class terminals_state : public driver_device
{
public:
	terminals_state(const machine_config &mconfig, device_type type, const char *tag)
		: driver_device(mconfig, type, tag)
		, m_maincpu(*this, "maincpu")
		//, m_p_chargen(*this, "chargen")
	{ }

private:
	required_device<cpu_device> m_maincpu;
	//required_region_ptr<u8> m_p_chargen;
};

static ADDRESS_MAP_START( mem_map, AS_PROGRAM, 8, terminals_state )
ADDRESS_MAP_END

static INPUT_PORTS_START( terminals )
INPUT_PORTS_END

static MACHINE_CONFIG_START( terminals )
	MCFG_CPU_ADD("maincpu", Z80, 2'000'000)
	MCFG_CPU_PROGRAM_MAP(mem_map)
MACHINE_CONFIG_END

/**************************************************************************************************************

Ann Arbor Ambassador.
Chips: Z80A, M58725P (16k RAM), 2x SCN2651C, nvram, button-battery
Crystals: 18.414, 6.0688

***************************************************************************************************************/

ROM_START( aaa )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "459_1.bin",    0x0000, 0x1000, CRC(55fb3e3b) SHA1(349cd257b1468827e1b389be7c989d0e4a13a5f1) )
	ROM_LOAD( "459_3.bin",    0x1000, 0x1000, CRC(e1e84ca4) SHA1(42dc5f4211beee79178f0c03bb45c66833119eae) )
	ROM_LOAD( "459_4.bin",    0x8000, 0x2000, CRC(4038aa89) SHA1(caf33c1f87aa396860324b9c73b35e4221f03d2e) )

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "202510b.bin",  0x0000, 0x1000, CRC(deda4aa4) SHA1(0bce5a8dc260ba51f3e431d8da408eac1f41acf7) )
ROM_END

COMP( 1981, aaa, 0, 0, terminals, terminals, terminals_state, 0, "Ann Arbor", "Ambassador", MACHINE_IS_SKELETON )



/**************************************************************************************************************

Qume QVT-70.
Chips: Z80, Z80 DART, 5x CXK5864CM-70LL/W242575-70LL, 801000-02, 303489-01, DS1231, Button battery, Beeper
Crystals: unreadable

***************************************************************************************************************/

ROM_START( qvt70 )
	ROM_REGION(0x30000, "maincpu", 0)
	ROM_LOAD( "251513-04_revj.u12", 0x00000, 0x10000, CRC(3960bbd5) SHA1(9db306cef09be21ff43c081ebe11e9b46f617861) )
	ROM_LOAD( "251513-03_revj.u11", 0x10000, 0x20000, CRC(c56796fe) SHA1(afe024ff93d5e75dc18041219d61e1a22fc6d883) )
ROM_END

COMP( 1992, qvt70, 0, 0, terminals, terminals, terminals_state, 0, "Qume", "QVT-70", MACHINE_IS_SKELETON )



/**************************************************************************************************************

Qume QVT-103.
Chips: Z80A, Z80A DART, Z80A CTC, 2x CRT9212, 5x HM6116P-2, TC5516APL, D8741AD, CRT9007, 1x 10-sw dip, Button battery.
Crystals: (all hard to read) 29.876, 6.000
Keyboard CPU, Crystal, ROM are on the main board.

***************************************************************************************************************/

ROM_START( qvt103 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "t103e1.u28", 0x0000, 0x2000, CRC(eace3cbe) SHA1(1e7f395c5233d8656df5305163d050275f0a8033) )
	ROM_LOAD( "t103e2.u27", 0x2000, 0x4000, CRC(100cf542) SHA1(4b2569d509790a0f94b4447fb9d3d42582fcaf66) )

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "c103b.u40",  0x0000, 0x1000, CRC(3419760d) SHA1(3455c70ed48c7f7769d73a84f152beddf508094f) )

	ROM_REGION(0x10000, "keyboard", 0)
	ROM_LOAD( "k304a.u24",  0x0000, 0x0400, CRC(e4b1f0da) SHA1(e9f8c48c34105464b3db206b34f67e7603484fea) )
ROM_END

COMP( 1983, qvt103, 0, 0, terminals, terminals, terminals_state, 0, "Qume", "QVT-103", MACHINE_IS_SKELETON )



/**************************************************************************************************************

Qume QVT-201.
Chips: Z80A, SCN2681A, SCN2672B, 4x HM6116P-2, D446C-2, button battery
Crystals: (from schematics, unreadable on photo) 48.654 MHz (Y1), 3.6864 MHz (Y2)
Board is marked QVT-202.

***************************************************************************************************************/

ROM_START( qvt201 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "390410-002.u11", 0x0000, 0x4000, CRC(69337561) SHA1(022e49bf5e8d76a3c2cc5af65630d3f77cc32bc1) )
	ROM_LOAD( "390410-001.u10", 0x4000, 0x4000, CRC(977cc138) SHA1(a019980ea6da2dce53617bced420014ab4e03ec8) )

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "301847-01.u42",  0x0000, 0x1000, CRC(546ed236) SHA1(312d57a7012f50327310bd11bda000149f13342e) )
ROM_END

COMP( 1986, qvt201, 0, 0, terminals, terminals, terminals_state, 0, "Qume", "QVT-201", MACHINE_IS_SKELETON )



/**************************************************************************************************************

Televideo TVI-955
Chips: G65SC02P-3, 3x S6551AP, SCN2674B, AMI 131406-00 (unknown 40-pin DIL), odd round silver thing, might be a battery
Crystals: 19.3396, 31.684, 3.6864
Keyboard: M5L8049-230P-6, 5.7143, Beeper

***************************************************************************************************************/

ROM_START( tv955 )
	ROM_REGION(0x10000, "maincpu", 0)
	ROM_LOAD( "t180002-88d_955.u4",  0x0000, 0x4000, CRC(5767fbe7) SHA1(49a2241612af5c3af09778ffa541ac0bc186e05a) )
	ROM_LOAD( "t180002-91a_calc.u5", 0x4000, 0x2000, CRC(f86c103a) SHA1(fa3ada3a5d8913e519e2ea4817e96166c1fedd32) ) // first half is all FF

	ROM_REGION(0x1000, "chargen", 0)
	ROM_LOAD( "t180002-26b.u45",     0x0000, 0x1000, CRC(69c9ebc7) SHA1(32282c816ec597a7c45e939acb7a4155d35ea584) )

	ROM_REGION(0x10000, "keyboard", 0)
	ROM_LOAD( "8049.kbd",            0x0000, 0x0800, CRC(bc86e349) SHA1(0b62003ab7931822f1bcac8370517c685849f62c) )
ROM_END

COMP( 1985, tv955, 0, 0, terminals, terminals, terminals_state, 0, "TeleVideo", "TVI-955", MACHINE_IS_SKELETON )



/**************************************************************************************************************

Televideo TVI-965
Chips: G65SC816P-5, SCN2672TC5N40, 271582-00 (unknown square chip), 2x UM6551A, Beeper
Crystals: 44.4528, 26.9892, 3.6864

***************************************************************************************************************/

ROM_START( tv965 )
	ROM_REGION(0x20000, "maincpu", 0)
	ROM_LOAD( "180003-30h.u8", 0x00000, 0x010000, CRC(c7b9ca39) SHA1(1d95a8b0a4ea5caf3fb628c44c7a3567700a0b59) )
	ROM_LOAD( "180003-38h.u9", 0x10000, 0x008000, CRC(30fae408) SHA1(f05bb2a9ce2df60b046733f746d8d8a1eb3ac8bc) )
ROM_END

COMP( 1989, tv965, 0, 0, terminals, terminals, terminals_state, 0, "TeleVideo", "TVI-965", MACHINE_IS_SKELETON )
