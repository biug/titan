//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "guardNatural.h"
#include "locutus.h"
#include "strategy.h"
#include "../units/army.h"
#include "../units/his.h"
#include "../Iron.h"

namespace { auto & bw = Broodwar; }




namespace iron
{


	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class GuardNatural
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	GuardNatural::GuardNatural()
	{
	}


	GuardNatural::~GuardNatural()
	{
		ai()->GetStrategy()->SetMinScoutingSCVs(1);
	}


	string GuardNatural::StateDescription() const
	{
		if (!m_active) return "-";
		if (m_active) return "active";

		return "-";
	}

	void GuardNatural::OnFrame_v()
	{
		if (m_DTDropByeFrame == 0)
			if (him().Player()->getName() == "locutus" || him().Player()->getName() == "Locutus")
				if (!ai()->GetStrategy()->Detected<Locutus>())
				{
					m_DTDropByeFrame = ai()->Frame();
				}

		if (m_DTDropByeFrame > 0)
			if (ai()->Frame() - m_DTDropByeFrame > 3 * 24)
				m_active = true;

		// 半队队坦克+3个雷车可出门
		if (me().SupplyUsed() >= 160)
		{
			m_active = false;
			Discard();
		}
	}


} // namespace iron



