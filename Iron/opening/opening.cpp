#include "opening.h"
#include "../units/me.h"
#include "../units/production.h"

namespace { auto & bw = Broodwar; }

namespace iron
{
	bool Opening::CheckRightExpert(Expert * e)
	{
		MacroItem item = FirstItem();
		if (item.IsTech())
		{
			if (e->GetType() == MacroItems::Tech)
				if (e->GetID() == item.GetTechType())
					return true;
		}
		else if (item.IsUpgrade())
		{
			if (e->GetType() == MacroItems::Upgrade)
				if (e->GetID() == item.GetTechType())
					return true;
		}
		else
		{
			if (e->GetType() == MacroItems::Unit)
				if (e->GetID() == item.GetUnitType())
					return true;
		}
		return false;
	}

	bool Opening::CheckRightKeepExpert(Expert * e, bool checkSupply) {
		if (e->GetType() == MacroItems::Unit)
		{
			unordered_map<int, int>::iterator it = m_keepProduceItems.find(e->GetID());
			if (it != m_keepProduceItems.end())
			{
				if (checkSupply && e->GetID() == Terran_Supply_Depot && m_keepProduceItems.size() > 1 &&
					(me().Buildings(Terran_Supply_Depot).size() * 8 + 10 > me().SupplyUsed() + 3))
					return false;
				return true;
			}
		}
		return false;
	}

	void Opening::DrawTextOnScreen(int xx)
	{
		if (IsAllEmpty()) return;
		int yy = 40;
		bw->drawTextScreen(xx, yy - 15, "Opening");
		for (const auto & item : m_openingItems)
		{
			bw->drawTextScreen(xx, yy, item.GetName().c_str());
			yy += 15;
		}
	}

	Opening GetOpeningByName(string name)
	{
		Opening opening;
		if (name == "8BB CC")
		{
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_Barracks)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Refinery)));
			opening.AddItem(MacroItem(UnitType(Terran_Command_Center), MacroLocation::Natural));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Factory)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Bunker), MacroLocation::Natural));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
		}
		else if (name == "SKT")
		{
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Barracks)));
			opening.AddItem(MacroItem(UnitType(Terran_Refinery)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_Factory)));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainSCV, 7));
			opening.AddItem(MacroItem(MacroCommandType::KeepBuildDepot, 2));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainMarine, 6));
			opening.AddItem(MacroItem(MacroCommandType::BuildMachineShop));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(UnitType(Terran_Factory)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainSCV, 7));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(MacroCommandType::KeepBuildDepot, 2));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainMarine, 5));
			opening.AddItem(MacroItem(UnitType(Terran_Engineering_Bay)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(UnitType(Terran_Missile_Turret)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(UnitType(Terran_Missile_Turret)));
			opening.AddItem(MacroItem(UnitType(Terran_Siege_Tank_Tank_Mode)));
			opening.AddItem(MacroItem(UnitType(Terran_Missile_Turret)));
			opening.AddItem(MacroItem(TechTypes::Tank_Siege_Mode));
			opening.AddItem(MacroItem(UnitType(Terran_Vulture)));
			opening.AddItem(MacroItem(UnitType(Terran_Vulture)));
		}
		else if (name == "5BB")
		{
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Barracks)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_Barracks)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(UnitType(Terran_SCV)));
			opening.AddItem(MacroItem(MacroCommandType::BarracksRallyNatural));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Supply_Depot)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(UnitType(Terran_Marine)));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainSCV, 4));
			opening.AddItem(MacroItem(MacroCommandType::KeepBuildDepot, 4));
			opening.AddItem(MacroItem(MacroCommandType::KeepBuildBarracks, 3));
			opening.AddItem(MacroItem(MacroCommandType::KeepTrainMarine, 30));
			
		}
		return opening;
	}

	void Opening::addKeepProduceItems(UnitType type, int amount)
	{
		if (m_keepProduceItems.find(type) != m_keepProduceItems.end())
			m_keepProduceItems[type] += amount;
		else
			m_keepProduceItems[type] = amount;
	}

	void Opening::finishOneItem(UnitType type)
	{
		m_keepProduceItems[type]--;
		//bw->sendText("%d: finish one item and left %d", type, m_keepProduceItems[type]);
		if (m_keepProduceItems[type] == 0)
			m_keepProduceItems.erase(type);
	}
}