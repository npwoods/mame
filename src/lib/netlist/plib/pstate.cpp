// license:GPL-2.0+
// copyright-holders:Couriersud
/*
 * pstate.c
 *
 */

#include "pstate.h"

PLIB_NAMESPACE_START()

pstate_manager_t::pstate_manager_t()
{
}

pstate_manager_t::~pstate_manager_t()
{
	m_save.clear();
}



ATTR_COLD void pstate_manager_t::save_state_ptr(const pstring &stname, const pstate_data_type_e dt, const void *owner, const int size, const int count, void *ptr, bool is_ptr)
{
	pstring fullname = stname;
	ATTR_UNUSED  pstring ts[] = {
			"NOT_SUPPORTED",
			"DT_CUSTOM",
			"DT_DOUBLE",
#if (PHAS_INT128)
			"DT_INT128",
#endif
			"DT_INT64",
			"DT_INT16",
			"DT_INT8",
			"DT_INT",
			"DT_BOOLEAN",
			"DT_FLOAT"
	};

	auto p = pmake_unique<pstate_entry_t>(stname, dt, owner, size, count, ptr, is_ptr);
	m_save.push_back(std::move(p));
}

ATTR_COLD void pstate_manager_t::remove_save_items(const void *owner)
{
	unsigned i = 0;
	while (i < m_save.size())
	{
		if (m_save[i]->m_owner == owner)
			m_save.remove_at(i);
		else
			i++;
	}
}

ATTR_COLD void pstate_manager_t::pre_save()
{
	for (std::size_t i=0; i < m_save.size(); i++)
		if (m_save[i]->m_dt == DT_CUSTOM)
			m_save[i]->m_callback->on_pre_save();
}

ATTR_COLD void pstate_manager_t::post_load()
{
	for (std::size_t i=0; i < m_save.size(); i++)
		if (m_save[i]->m_dt == DT_CUSTOM)
			m_save[i]->m_callback->on_post_load();
}

template<> ATTR_COLD void pstate_manager_t::save_item(pstate_callback_t &state, const void *owner, const pstring &stname)
{
	//save_state_ptr(stname, DT_CUSTOM, 0, 1, &state);
	pstate_callback_t *state_p = &state;
	auto p = pmake_unique<pstate_entry_t>(stname, owner, state_p);
	m_save.push_back(std::move(p));
	state.register_state(*this, stname);
}

PLIB_NAMESPACE_END()
