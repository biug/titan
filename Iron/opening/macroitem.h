#ifndef MACROITEM_H
#define MACROITEM_H

#include <BWAPI.h>
#include "macrocommand.h"

namespace iron
{
	enum class MacroLocation
	{
		Anywhere     // default location
		, Natural      // "natural" first expansion base
		, Main			// main base
	};

	namespace MacroItems
	{
		enum { Unit, Tech, Upgrade, Command, Default };
	}

	class MacroItem
	{
		size_t				m_type;
		Race				m_race;
		string				m_name;

		UnitType			m_unitType;
		TechType			m_techType;
		UpgradeType			m_upgradeType;
		MacroCommand		m_macroCommandType;

		MacroLocation		m_macroLocation;

		MacroLocation		GetMacroLocationFromString(string & s);

	public:

		MacroItem();
		MacroItem(const string & name);
		MacroItem(UnitType t);
		MacroItem(UnitType t, MacroLocation loc);
		MacroItem(TechType t);
		MacroItem(UpgradeType t);
		MacroItem(MacroCommandType t);
		MacroItem(MacroCommandType t, int amount);

		bool    IsUnit()			const;
		bool    IsTech()			const;
		bool    IsUpgrade()			const;
		bool    IsCommand()			const;
		bool    IsBuilding()		const;
		bool	IsBuildingAddon()	const;
		bool    IsRefinery()		const;
		bool	IsSupply()			const;

		const size_t & type()		const;
		const Race & GetRace()		const;

		const UnitType & GetUnitType()			const;
		const TechType & GetTechType()			const;
		const UpgradeType & GetUpgradeType()	const;
		const MacroCommand GetCommandType()		const;
		const MacroLocation GetMacroLocation()	const;

		int supplyRequired()	const;
		int mineralPrice()		const;
		int gasPrice()			const;
		UnitType whatBuilds()	const;
		string GetName()		const;
	};
}

#endif