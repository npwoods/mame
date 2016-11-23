// license:BSD-3-Clause
// copyright-holders:Nathan Woods
/***************************************************************************

    ui/bbcontrl.h

    MESS's "bit banger" control

    Copyright Nicola Salmoria and the MAME Team.
    Visit http://mamedev.org for licensing and usage restrictions.

***************************************************************************/

#pragma once

#ifndef __UI_BBCONTRL_H__
#define __UI_BBCONTRL_H__

#include "imagedev/bitbngr.h"
#include "ui/devctrl.h"

namespace ui {

class ui_menu_bitbanger_control : public menu_device_control<bitbanger_device> {
public:
	ui_menu_bitbanger_control(mame_ui_manager &mui, render_container &container, bitbanger_device *bitbanger);
	virtual ~ui_menu_bitbanger_control();
	virtual void populate(float &customtop, float &custombottom) override;
	virtual void handle() override;
};

} // namespace ui

#endif // __UI_BBCONTRL_H__
