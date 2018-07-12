//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "SKT.h"
#include "strategy.h"
#include "expand.h"
#include "../units/army.h"
#include "../units/his.h"
#include "../Iron.h"

namespace { auto & bw = Broodwar; }

namespace iron
{


	//////////////////////////////////////////////////////////////////////////////////////////////
	//                                                                                          //
	//                                  class Locutus
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	SKT::SKT()
	{
		std::string enemyName = him().Player()->getName();
		if (enemyName == "Locutus" || enemyName == "locutus")
		{
			me().SetOpening("SKT");
			m_active = true;
		}
	}


	SKT::~SKT()
	{
		bw->sendText("skt end");
		ai()->GetStrategy()->SetMinScoutingSCVs(1);
	}


	string SKT::StateDescription() const
	{
		if (!m_active) return "-";
		if (m_active) return "active";

		return "-";
	}

	void SKT::OnFrame_v()
	{
		// 12分钟后，取消skt，或者消灭基地后
		if (him().LostUnits(Protoss_Nexus) > 0 || ai()->Frame() > 11200)
		{
			m_active = false;
			bw->sendText("SKT end");
			Discard();
		}
	}


} // namespace iron



