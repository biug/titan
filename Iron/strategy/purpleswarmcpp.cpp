//////////////////////////////////////////////////////////////////////////
//
// This file is part of Iron's source files.
// Iron is free software, licensed under the MIT/X11 License. 
// A copy of the license is provided with the library in the LICENSE file.
// Copyright (c) 2016, 2017, Igor Dimitrijevic
//
//////////////////////////////////////////////////////////////////////////


#include "purpleswarm.h"
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
	//                                  class Purpleswarm
	//                                                                                          //
	//////////////////////////////////////////////////////////////////////////////////////////////


	Purpleswarm::Purpleswarm()
	{
		std::string enemyName = him().Player()->getName();
		if (him().Race() == BWAPI::Races::Zerg &&
			(enemyName == "Purpleswarm" || enemyName == "purpleswarm" || enemyName == "PurpleSwarm"))
		{
			me().SetOpening("5BB");
			m_detected = true;
		}
	}


	Purpleswarm::~Purpleswarm()
	{
		ai()->GetStrategy()->SetMinScoutingSCVs(1);
	}


	string Purpleswarm::StateDescription() const
	{
		if (!m_detected) return "-";
		if (m_detected) return "detected";

		return "-";
	}

	void Purpleswarm::OnFrame_v()
	{

		if (!m_pNaturalArea)
			m_pNaturalArea = findNatural(me().StartingVBase())->BWEMPart()->GetArea();
		bool hasEnemyBuildingsInNatural = any_of(him().Buildings().begin(), him().Buildings().end(), [](const unique_ptr<HisBuilding> & b)
		{ return b->Completed() && contains(ext(me().NaturalArea())->EnlargedArea(), b->GetArea(check_t::no_check)); });

		//discard when no enermy base at natrual 
		if (m_detected)
		{
			if (me().Units(Terran_Vulture).size() > 2 && !hasEnemyBuildingsInNatural)
			{
				bw->sendText("on frame%d discard purpleswarm", ai()->Frame());
				m_detected = false;
				Discard();
				return;
			}
		}
		else
		{
			if (bw->getFrameCount() < 24 * 2 * 60 && hasEnemyBuildingsInNatural)
			{
				m_detected = true;
				return;
			}
		}
	}


} // namespace iron



