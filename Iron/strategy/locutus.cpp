//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "locutus.h"
#include "strategy.h"
#include "zerglingRush.h"
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


	Locutus::Locutus()
	{
		std::string enemyName = him().Player()->getName();
		if (enemyName == "Locutus" || enemyName == "locutus")
		{
			me().SetOpening("SKT");
			m_detected = true;
		}
	}


	Locutus::~Locutus()
	{
		ai()->GetStrategy()->SetMinScoutingSCVs(1);
	}


	string Locutus::StateDescription() const
	{
		if (!m_detected) return "-";
		if (m_detected) return "detected";

		return "-";
	}

	void Locutus::OnFrame_v()
	{
		if (him().LostUnits(Protoss_Shuttle) == 1 && him().Units(Protoss_Dark_Templar).size() == 0)
		{
			m_detected = false;
			Discard();
			return;
		}
		if (him().LostUnits(Protoss_Dark_Templar) >= 4)
		{
			m_detected = false;
			Discard();
			return;
		}
		if (bw->getFrameCount() > 13000)
		{
			m_detected = false;
			Discard();
			return;
		}
	}


} // namespace iron



